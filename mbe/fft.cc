#include "fft.h"

#include "tables.h"

namespace halfrate::core {

void fft512(float* re, float* im) {
	// bit-reversal permutation
	unsigned j = 0;
	for (int i = 0; i < 512; i++) {
		if (i < static_cast<int>(j)) {
			float r = re[i], m = im[i];
			re[i] = re[j];
			im[i] = im[j];
			re[j] = r;
			im[j] = m;
		}
		unsigned bit = 0x100;
		while (bit && (j & bit)) {
			j ^= bit;
			bit >>= 1;
		}
		j |= bit;
	}
	const float* cosine = tables::kCosine;
	for (int size = 2, shift = 1; size <= 512; size <<= 1, shift++) {
		int half = size >> 1;
		unsigned step = 8192u >> shift;  // table entries per twiddle step
		for (int start = 0; start < 512; start += size) {
			for (int k = 0; k < half; k++) {
				float c = cosine[k * step];
				float s = cosine[(6144 + k * step) & 8191];  // sin
				int top = start + k, bottom = start + half + k;
				float xr = re[bottom], xi = im[bottom];
				// t = x e^-j(angle) = (c xr + s xi) + j (c xi - s xr)
				float tr = c * xr;
				tr = tr + s * xi;
				float ti = c * xi;
				ti = ti - s * xr;
				re[bottom] = re[top] - tr;
				im[bottom] = im[top] - ti;
				re[top] = tr + re[top];
				im[top] = ti + im[top];
			}
		}
	}
}

}  // namespace halfrate::core
