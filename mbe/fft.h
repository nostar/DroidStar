// 512-point complex FFT.
#pragma once

namespace halfrate::core {

// In-place forward transform (kernel e^-j), unscaled, of re[512] + j im[512]: radix 2, decimation in time, with
// twiddles from the 8192-entry cosine table.
void fft512(float* re, float* im);

}  // namespace halfrate::core
