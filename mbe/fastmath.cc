#include "fastmath.h"

#include <cstdint>

#include "numeric.h"

namespace halfrate::core {

namespace {
constexpr float kSmallestNormal = 0x1p-126f;
constexpr float kSqrt2 = 0x1.6a09e6p+0f;        // 1.41421354
constexpr float kTwoOverLn2 = 0x1.715476p+1f;   // 2 / ln 2
constexpr float kLn2 = 0x1.62e43p-1f;           // ln 2
}  // namespace

float fast_log2(float x) {
	if (!(x >= kSmallestNormal)) return -126.0f;
	uint32_t bits = float_bits(x);
	float m = float_from_bits(0x3f800000u | (bits & 0x7fffffu));  // mantissa in [1, 2)
	float half = m * 0.5f;
	bool fold = m > kSqrt2;
	m = fold ? half : m;
	int exponent = (fold ? -126 : -127) + static_cast<int>(bits >> 23);
	// log(m) = 2 atanh(t), t = (m - 1) / (m + 1); series t (1 + t^2/3 + t^4/5 + t^6/7 + t^8/9)
	float t = (m + -1.0f) / (m + 1.0f);
	float t2 = t * t;
	float p = t2 * (1.0f / 9.0f);
	p = p + 1.0f / 7.0f;
	p = t2 * p;
	p = p + 1.0f / 5.0f;
	p = t2 * p;
	p = p + 1.0f / 3.0f;
	p = t2 * p;
	p = p + 1.0f;
	float r = t * p;
	r = r * kTwoOverLn2;
	return r + static_cast<float>(exponent);
}

float fast_exp2(float x) {
	if (x != x) return 0.0f;
	x = x < -126.0f ? -126.0f : x;
	x = x > 127.0f ? 127.0f : x;
	// round half up: x + 200.5 is positive, so truncation is floor
	int whole = static_cast<int>(x + 0.5f + 200.0f) - 200;
	float f = x - static_cast<float>(whole);
	f = f * kLn2;
	// e^f = 1 + f (1 + f (1/2 + f (1/6 + f (1/24 + f (1/120 + f (1/720 + f / 5040))))))
	float p = f * (1.0f / 5040.0f);
	p = p + 1.0f / 720.0f;
	p = f * p;
	p = p + 1.0f / 120.0f;
	p = f * p;
	p = p + 1.0f / 24.0f;
	p = f * p;
	p = p + 1.0f / 6.0f;
	p = f * p;
	p = p + 0.5f;
	p = f * p;
	p = p + 1.0f;
	float r = f * p;
	r = r + 1.0f;
	whole = whole > -126 ? whole : -126;
	whole = whole < 127 ? whole : 127;
	return r * float_from_bits(0x3f800000u + (static_cast<uint32_t>(whole) << 23));
}

float fast_sqrt(float x) {
	if (!(x > kSmallestNormal)) return 0.0f;
	return fast_exp2(fast_log2(x) * 0.5f);
}

}  // namespace halfrate::core
