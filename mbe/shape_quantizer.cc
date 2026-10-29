#include "shape_quantizer.h"

#include "constants.h"
#include "tables.h"

namespace halfrate::enc {

namespace {

// Element `element`, basis function `order` of the size-n inverse DCT.
inline float inverse_dct(int n, int element, int order) { return tables::kInverseDct[n - 1][element][order]; }

// Inserts (value, id) into the ascending list of at most capacity entries.
void insert_best(float* values, int* ids, int* count, int capacity, float value, int id) {
	int n = *count;
	if (n == capacity && value >= values[capacity - 1]) return;
	int pos;
	if (n < capacity) {
		*count = n + 1;
		pos = n;
	} else {
		pos = capacity - 1;
	}
	while (pos >= 1 && values[pos - 1] > value) {
		values[pos] = values[pos - 1];
		ids[pos] = ids[pos - 1];
		pos--;
	}
	values[pos] = value;
	ids[pos] = id;
}

// a . b over 4 lanes, summed ((0 + p0) + p1 + p2) + p3 from a zero start
inline float dot4_from_zero(const float* a, const float* b) {
	float p[4];
	for (int j = 0; j < 4; j++) p[j] = a[j] * b[j];
	float s = p[0] + 0.0f;
	s = s + p[1];
	s = s + p[2];
	s = s + p[3];
	return s;
}

// a . b over 4 lanes, summed ((p0 + p1) + p2) + p3
inline float dot4(const float* a, const float* b) {
	float p0 = a[0] * b[0], p1 = a[1] * b[1], p2 = a[2] * b[2], p3 = a[3] * b[3];
	float s = p0 + p1;
	s = s + p2;
	s = s + p3;
	return s;
}

}  // namespace

void init_shape_basis(ShapeBasis& basis, const core::Codebook& codebook) {
	basis.offset.fill(0);
	basis.values.fill(0.0f);
	int offset = 0;
	for (int L = kMinBasisHarmonics; L <= kMaxBasisHarmonics; L++) {
		basis.offset[L] = offset;
		float* rows = &basis.values[offset];
		const int32_t* blocks = codebook.band_sizes[L];
		// rows 0..6: coefficient 1 + row of the 8-point transform, through each block's pair (sum, difference)
		for (int row = 0; row < 7; row++) {
			float pair[4][2];
			for (int i = 0; i < 4; i++) {
				pair[i][0] = inverse_dct(8, 2 * i, 1 + row);
				pair[i][1] = inverse_dct(8, 2 * i + 1, 1 + row);
			}
			int pos = 0;
			for (int b = 0; b < 4; b++) {
				int n = blocks[b];
				if (n < 1) continue;
				float sum = (pair[b][0] + pair[b][1]) * 0.5f;
				float difference = (pair[b][0] - pair[b][1]) * kHalfInvSqrt2;
				float* out = rows + row * L + pos;
				pos += n;
				for (int e = 0; e < n; e++) {
					float v = sum * inverse_dct(n, e, 0);
					if (n != 1) v = v + difference * inverse_dct(n, e, 1);
					out[e] = v;
				}
			}
		}
		// rows 7 + 4 b + r: order 2 + r of block b's inverse DCT
		int pos = 0;
		for (int b = 0; b < 4; b++) {
			int n = blocks[b];
			for (int r = 0; r < 4; r++) {
				if (r + 2 >= n) continue;
				float* out = rows + (7 + 4 * b + r) * L + pos;
				for (int e = 0; e < n; e++) out[e] = inverse_dct(n, e, 2 + r);
			}
			pos = n + pos;
		}
		float reciprocal = tables::kReciprocalCount[L];
		for (int row = 0; row < kBasisRows; row++) {
			float* out = rows + row * L;
			float s = 0.0f;
			for (int i = 0; i < L; i++) s = s + out[i];
			s = s * reciprocal;
			for (int i = 0; i < L; i++) out[i] = out[i] - s;
		}
		offset = offset + kBasisRows * L;
	}
}

int search_shape(const ShapeBasis& basis, const core::Codebook& codebook, int L, const float* x, const float* w,
                 int best_low, int best_high, ShapeCandidate* out, int result_count) {
	best_low = best_low > 1 ? best_low : 1;
	best_low = best_low < 16 ? best_low : 16;
	best_high = best_high > 1 ? best_high : 1;
	best_high = best_high < 16 ? best_high : 16;
	const float* rows = basis.rows(L);
	float wx[kMaxBasisHarmonics];
	float gram[kBasisRows][kBasisRows], correlation[kBasisRows];
	float low_energy[512], high_energy[128], detail_energy[4][32];
	float low_values[16], high_values[16], residual1[kBasisRows], residual3[16];
	int low_ids[16], high_ids[16], low_count = 0, detail[4];
	const float* chosen[4];

	float energy = 0.0f;
	for (int i = 0; i < L; i++) {
		wx[i] = w[i] * x[i];
		energy = energy + x[i] * wx[i];
	}
	for (int r = 0; r < kBasisRows; r++) {
		const float* row = rows + r * L;
		float s = 0.0f;
		for (int i = 0; i < L; i++) s = s + row[i] * wx[i];
		correlation[r] = s;
		for (int t = 0; t <= r; t++) {
			const float* other = rows + t * L;
			float g = 0.0f;
			for (int i = 0; i < L; i++) g = g + w[i] * row[i] * other[i];
			gram[r][t] = g;
			gram[t][r] = g;
		}
	}
	const float(*low_book)[3] = codebook.shape_low;
	const float(*high_book)[4] = codebook.shape_high;
	for (int j = 0; j < 512; j++) {
		const float* p = low_book[j];
		float acc = 0.0f;
		for (int a = 0; a < 3; a++) {
			float t = 0.0f;
			for (int b = 0; b < 3; b++) t = t + gram[a][b] * p[b];
			acc = acc + t * p[a];
		}
		low_energy[j] = acc;
	}
	for (int j = 0; j < 128; j++) {
		const float* p = high_book[j];
		float acc = 0.0f;
		for (int a = 0; a < 4; a++) acc = acc + dot4_from_zero(&gram[3 + a][3], p) * p[a];
		high_energy[j] = acc;
	}
	for (int b = 0; b < 4; b++) {
		int n = codebook.detail_count[b];
		const float* book = codebook.detail[b];
		long stride = codebook.detail_stride[b];
		for (int e = 0; e < n; e++) {
			const float* p = book + e * stride * 4;
			float acc = 0.0f;
			for (int a = 0; a < 4; a++) acc = acc + dot4_from_zero(&gram[7 + 4 * b + a][7 + 4 * b], p) * p[a];
			detail_energy[b][e] = acc;
		}
	}
	// stage 1: shape-low
	for (int j = 0; j < 512; j++) {
		const float* p = low_book[j];
		float t = p[0] * correlation[0];
		float u1 = p[1] * correlation[1], u2 = p[2] * correlation[2];
		t = t + u1;
		t = t + u2;
		t = t + t;
		insert_best(low_values, low_ids, &low_count, best_low, (energy + low_energy[j]) - t, j);
	}
	int results = 0;
	for (int i1 = 0; i1 < low_count; i1++) {
		int j1 = low_ids[i1];
		const float* p1 = low_book[j1];
		int high_count = 0;
		for (int r = 3; r < kBasisRows; r++) {
			float s = correlation[r];
			for (int a = 0; a < 3; a++) s = s - gram[r][a] * p1[a];
			residual1[r] = s;
		}
		float v1 = low_values[i1];
		// stage 2: shape-high
		for (int j = 0; j < 128; j++) {
			float t = dot4(high_book[j], &residual1[3]);
			t = t + t;
			insert_best(high_values, high_ids, &high_count, best_high, (v1 + high_energy[j]) - t, j);
		}
		for (int i2 = 0; i2 < high_count; i2++) {
			int j2 = high_ids[i2];
			const float* p2 = high_book[j2];
			for (int r = 0; r < 16; r++) {
				float s = residual1[7 + r];
				for (int a = 0; a < 4; a++) s = s - gram[7 + r][3 + a] * p2[a];
				residual3[r] = s;
			}
			// stage 3: each block's detail on its own
			for (int b = 0; b < 4; b++) {
				int n = codebook.detail_count[b], arg = 0;
				const float* book = codebook.detail[b];
				int stride = codebook.detail_stride[b];
				if (n > 0) {
					float best = kHuge;
					for (int e = 0; e < n; e++) {
						float t = dot4(book + static_cast<long>(e) * stride * 4, &residual3[4 * b]);
						t = t + t;
						float s = detail_energy[b][e] - t;
						if (s < best) {
							best = s;
							arg = e;
						}
					}
				}
				detail[b] = arg;
				chosen[b] = book + static_cast<long>(stride * arg) * 4;
			}
			float distortion = high_values[i2];
			for (int r = 0; r < 16; r++) {
				float s = residual3[r] * -2.0f;
				for (int t = 0; t < 16; t++) s = s + gram[7 + r][7 + t] * chosen[t >> 2][t & 3];
				s = s * chosen[r >> 2][r & 3];
				distortion = distortion + s;
			}
			// insert into the sorted results
			int pos, count;
			if (results < result_count) {
				count = results + 1;
				pos = results;
			} else {
				count = results;
				if (!(distortion < out[results - 1].distortion)) continue;
				pos = result_count - 1;
			}
			while (pos >= 1 && out[pos - 1].distortion > distortion) {
				out[pos] = out[pos - 1];
				pos--;
			}
			ShapeCandidate& c = out[pos];
			c = ShapeCandidate{};
			c.low = j1;
			c.high = j2;
			for (int b = 0; b < 4; b++) c.detail[b] = detail[b];
			c.distortion = distortion;
			results = count;
		}
	}
	return results;
}

}  // namespace halfrate::enc
