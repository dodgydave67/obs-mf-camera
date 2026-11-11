// SPDX-License-Identifier: GPL-2.0-or-later
#include "../scale.hpp"
#include <array>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

int main()
{
	// Padding is deliberately distinct from image data in both planes.
	const uint8_t y[] = {1, 2, 3, 4, 99, 99, 5, 6, 7, 8, 99, 99,
			     9, 10, 11, 12, 99, 99, 13, 14, 15, 16, 99, 99};
	const uint8_t uv[] = {21, 31, 22, 32, 98, 98, 98, 98, 23, 33, 24, 34, 98, 98, 98, 98};
	std::array<uint8_t, 24> same{};
	obs_mf_scale_nv12(same.data(), 4, 4, y, uv, 4, 4, 6, 8);
	for (size_t i = 0; i < 16; ++i)
		check(same[i] == i + 1, "identity scale included Y padding");
	check(same[16] == 21 && same[17] == 31 && same[20] == 23 && same[23] == 34,
	      "identity scale included UV padding or swapped channels");
	std::array<uint8_t, 6> small{};
	obs_mf_scale_nv12(small.data(), 2, 2, y, uv, 4, 4, 6, 8);
	check(small == std::array<uint8_t, 6>{1, 3, 9, 11, 21, 31}, "downscale sampled wrong rows/pairs");
	std::array<uint8_t, 96> large{};
	obs_mf_scale_nv12(large.data(), 8, 8, y, uv, 4, 4, 6, 8);
	check(large[0] == 1 && large[1] == 1 && large[2] == 2 && large[63] == 16,
	      "upscale did not replicate luma pixels");
	check(large[64] == 21 && large[65] == 31 && large[66] == 21 && large[67] == 31 &&
		      large[94] == 24 && large[95] == 34,
	      "upscale split chroma pairs");
	std::cout << "NV12 stride, identity, upscale and downscale checks passed\n";
}
