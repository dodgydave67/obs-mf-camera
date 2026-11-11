// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

#define OBS_MF_WIDTH 1280
#define OBS_MF_HEIGHT 720
#define OBS_MF_FRAME_BYTES (OBS_MF_WIDTH * OBS_MF_HEIGHT * 3 / 2)
#define OBS_MF_MAGIC 0x464d424fU
#define OBS_MF_PIPE_PREFIX L"\\\\.\\pipe\\OBS-MF-"
#define OBS_MF_SOURCE_CLSID L"{BD34B55A-51CA-4A96-BEB4-601C35D572A9}"

#ifdef __cplusplus
#include <guiddef.h>
inline constexpr GUID OBS_MF_PIPE_NAME = {
	0x33a60c6e, 0x70ad, 0x412f, {0xb4, 0xa0, 0xba, 0xc5, 0xb7, 0x3b, 0xe5, 0xaa}};
#pragma pack(push, 1)
struct obs_mf_frame_header {
	uint32_t magic = OBS_MF_MAGIC;
	uint32_t version = 1;
	uint32_t bytes = OBS_MF_FRAME_BYTES;
	uint64_t sequence = 0;
};
#pragma pack(pop)
static_assert(sizeof(obs_mf_frame_header) == 20);
#endif
