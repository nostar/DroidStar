// The codec's own log2, exp2 and square root. They are approximations (series on the mantissa), and the codec's
// output depends on their exact results, so the codec uses these, never the C library's.
#pragma once

namespace halfrate::core {

// log2(x): an atanh series on the mantissa folded into [sqrt(1/2), sqrt(2)). For x below the smallest normal
// float (including 0, negatives, subnormals and NaN) it returns -126. log2(+inf) is 128.
float fast_log2(float x);

// 2^x: x clamped to [-126, 127], the integer part (rounded half up) into the exponent, and a degree-7 Taylor
// series of e^(f ln 2) for the rest. NaN gives 0.
float fast_exp2(float x);

// sqrt(x) = fast_exp2(fast_log2(x) / 2) for x above the smallest normal float; 0 otherwise (and for NaN).
float fast_sqrt(float x);

}  // namespace halfrate::core
