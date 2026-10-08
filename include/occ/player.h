/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_PLAYER_H
#define OCC_PLAYER_H
#include "occ.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Capability claims must be independently measured by the hardware adapter.
 * Declared capability is not electrical safety or output proof.
 * logical_channels: independently controllable outputs, not physical pin IDs.
 * min_interval_ns: minimum reliably schedulable transition interval.
 * jitter_ns: measured worst-case timing jitter, not nominal timer resolution. */
typedef struct {
    uint8_t logical_channels;
    uint64_t min_interval_ns;
    uint64_t jitter_ns;
} occ_led_capabilities;
typedef struct {
    const occ_wave *wave; /* immutable for player lifetime */
    size_t index;
    uint64_t start_ns,offset_ns,next_deadline_ns,max_lateness_ns;
    uint8_t finished,aborted;
} occ_player;
occ_result occ_player_init(occ_player *p,const occ_wave *wave,
                          const occ_led_capabilities *caps,uint64_t start_ns,
                          uint64_t max_lateness_ns);
/* No hardware calls. Adapter applies returned channel mask and schedules next
 * call at next_deadline_ns. Excessive lateness aborts, returns all-off.
 * Idle-before-start returns zero with start as next deadline.
 * Completion returns zero with UINT64_MAX as next deadline.
 * Output requires hardware-safe channel mapping/current configuration elsewhere. */
occ_result occ_player_step(occ_player *p,uint64_t now_ns,uint8_t *channels,
                          uint64_t *next_deadline_ns);
#ifdef __cplusplus
}
#endif
#endif
