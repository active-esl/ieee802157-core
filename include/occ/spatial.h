/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_SPATIAL_H
#define OCC_SPATIAL_H
#include "image.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Disjoint tracked ROIs only. Returns Figure179 phase bit when both sources
 * classify as binary; mixed exposure/occlusion/clipping produces erasure.
 * Simultaneous ROI timestamps are assumed. Rolling-row skew must be measured
 * by the adapter; this function cannot infer it from grayscale pixels. */
occ_result occ_s2psk_image_sample(const occ_gray_image *image,
                                 const occ_roi *a,const occ_roi *b,
                                 const occ_thresholds *thresholds,int *sample);
#define OCC_SYMBOL_READY 1
typedef struct {
    uint64_t epoch_ns,period_ns,max_capture_gap_ns,last_capture_ns,slot;
    uint16_t count,min_samples;
    uint8_t have_slot,bad,decision;
} occ_symbol_group;
/* Calibrated epoch/period grouping, NOT blind clock recovery.
 * Repeated camera frames must have genuine advancing capture timestamps.
 * Every symbol needs multiple consistent confident samples. */
occ_result occ_symbol_group_init(occ_symbol_group *g,uint64_t epoch_ns,
                                 uint64_t period_ns,uint64_t max_capture_gap_ns,
                                 uint16_t min_samples);
int occ_symbol_group_push(occ_symbol_group *g,uint64_t capture_ns,int sample,
                           uint64_t *symbol_ns,int *symbol);
int occ_symbol_group_flush(occ_symbol_group *g,uint64_t end_ns,
                            uint64_t *symbol_ns,int *symbol);
#ifdef __cplusplus
}
#endif
#endif
