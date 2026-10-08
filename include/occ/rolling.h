/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_ROLLING_H
#define OCC_ROLLING_H
#include "cook.h"
#include "image.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t optical_period_ns,row_period_ns,exposure_ns;
    unsigned min_confident_rows;
} occ_rolling_profile;
/* Portable calibrated row reconstruction + C-OOK fragment assembly.
 * Camera acquisition, source tracking and optical epoch calibration are external.
 * first_row_ns is the first ROI row's exposure START, not a container timestamp.
 * Each image reacquires complete repeated subpackets. No interframe partial
 * subpacket fusion. At most one completed envelope is returned per image.
 * Timing is explicit; no OS, LED driver or camera API is called. */
int occ_cook_decode_image(occ_cook_assembler *assembler,
        const occ_gray_image *image,const occ_roi *source,
        const occ_thresholds *thresholds,const occ_rolling_profile *profile,
        uint64_t first_row_ns,uint64_t optical_epoch_ns,
        occ_packet *packet,unsigned *uncertain);
#ifdef __cplusplus
}
#endif
#endif
