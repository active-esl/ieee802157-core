/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_IMAGE_H
#define OCC_IMAGE_H
#include "receiver.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const uint8_t *data;
    size_t width,height,stride;
} occ_gray_image;
typedef struct { size_t x,y,width,height; } occ_roi;
typedef struct {
    uint8_t low_max, high_min, mid_min, mid_max;
    uint8_t clipped_low, clipped_high;
    uint16_t max_clipped_per_mille;
} occ_thresholds;
/* Caller supplies calibrated grayscale frames, a tracked ROI and thresholds.
 * No auto exposure, white balance, camera ownership or source tracking implied.
 * A clipped ROI is an erasure, not proof of a confident ON state. */
occ_result occ_gray_classify(const occ_gray_image *image, const occ_roi *roi,
                            const occ_thresholds *thresholds, int *sample);
#ifdef __cplusplus
}
#endif
#endif
