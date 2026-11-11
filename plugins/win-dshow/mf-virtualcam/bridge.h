// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
struct obs_mf_camera;
struct obs_mf_camera *obs_mf_camera_start(uint32_t width, uint32_t height, long *error);
void obs_mf_camera_frame(struct obs_mf_camera *camera, uint8_t **planes, uint32_t *strides);
void obs_mf_camera_stop(struct obs_mf_camera *camera);
#ifdef __cplusplus
}
#endif
