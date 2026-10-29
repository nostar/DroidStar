// Numeric helpers with the conversion and rounding semantics the codec depends on.
//
// Float to integer conversions truncate toward zero and saturate at the target type's range; NaN converts to 0.
// (A plain C++ cast is undefined out of range; these are defined everywhere.) Rounding helpers name the rounding
// they do. Everything here is exact: no approximation, no rounding beyond what the name says.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace halfrate::core {

// The bits of a float and back.
inline uint32_t float_bits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
inline float float_from_bits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// Truncate toward zero, saturating; NaN gives 0.
inline int32_t to_int32_sat(float x) {
	if (x != x) return 0;
	if (x >= 2147483648.0f) return std::numeric_limits<int32_t>::max();
	if (x <= -2147483648.0f) return std::numeric_limits<int32_t>::min();
	return static_cast<int32_t>(x);
}
inline int32_t to_int32_sat(double x) {
	if (x != x) return 0;
	if (x >= 2147483648.0) return std::numeric_limits<int32_t>::max();
	if (x <= -2147483648.0) return std::numeric_limits<int32_t>::min();
	return static_cast<int32_t>(x);
}
inline int64_t to_int64_sat(float x) {
	if (x != x) return 0;
	if (x >= 9223372036854775808.0f) return std::numeric_limits<int64_t>::max();
	if (x <= -9223372036854775808.0f) return std::numeric_limits<int64_t>::min();
	return static_cast<int64_t>(x);
}
inline int64_t to_int64_sat(double x) {
	if (x != x) return 0;
	if (x >= 9223372036854775808.0) return std::numeric_limits<int64_t>::max();
	if (x <= -9223372036854775808.0) return std::numeric_limits<int64_t>::min();
	return static_cast<int64_t>(x);
}

// Integer results of the four roundings, saturating (NaN gives 0).
inline int32_t floor_to_int32(float x) { return to_int32_sat(std::floor(x)); }
inline int32_t ceil_to_int32(float x) { return to_int32_sat(std::ceil(x)); }
inline int32_t round_half_away_to_int32(float x) { return to_int32_sat(std::round(x)); }
inline int32_t round_half_even_to_int32(float x) { return to_int32_sat(std::nearbyint(x)); }

// Rounding to a whole float.
inline float floor_f(float x) { return std::floor(x); }
inline float ceil_f(float x) { return std::ceil(x); }
inline float trunc_f(float x) { return std::trunc(x); }
inline float round_half_away_f(float x) { return std::round(x); }  // 2.5 -> 3, -2.5 -> -3
inline float round_half_even_f(float x) { return std::nearbyint(x); }  // default rounding mode: 2.5 -> 2

// Minimum and maximum that ignore a NaN operand (IEEE minNum / maxNum).
inline float min_num(float a, float b) { return std::fmin(a, b); }
inline float max_num(float a, float b) { return std::fmax(a, b); }

}  // namespace halfrate::core
