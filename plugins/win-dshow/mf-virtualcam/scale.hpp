// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>

// Packed NV12 destination, independently pitched source planes. Dimensions must
// be nonzero and even; UV pairs remain together under nearest-neighbor scaling.
inline void obs_mf_scale_nv12(uint8_t *destination, uint32_t dw, uint32_t dh, const uint8_t *y_plane,
			     const uint8_t *uv_plane, uint32_t sw, uint32_t sh, uint32_t y_stride,
			     uint32_t uv_stride)
{
	for (uint32_t y = 0; y < dh; ++y) {
		const auto *source = y_plane + size_t(y * uint64_t(sh) / dh) * y_stride;
		for (uint32_t x = 0; x < dw; ++x)
			destination[size_t(y) * dw + x] = source[x * uint64_t(sw) / dw];
	}
	destination += size_t(dw) * dh;
	for (uint32_t y = 0; y < dh / 2; ++y) {
		const auto *source = uv_plane + size_t(y * uint64_t(sh) / dh) * uv_stride;
		for (uint32_t x = 0; x < dw; x += 2) {
			uint32_t sx = uint32_t(x * uint64_t(sw) / dw) & ~1U;
			destination[size_t(y) * dw + x] = source[sx];
			destination[size_t(y) * dw + x + 1] = source[sx + 1];
		}
	}
}
