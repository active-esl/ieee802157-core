/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_COLOUR_H
#define OCC_COLOUR_H
#include "receiver.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Custom slow-colour profile, NOT IEEE CSK/UFSOOK. One package, four
 * calibrated colours, one logical output active at a time. No hardware calls.
 * 16 sync + 88 dibits + 4 trailer + 2 dark = 110 symbols = 13.75s at 125ms.
 * Byte dibits are serialized least-significant first. */
#define OCC_COLOUR_SYNC 16U
#define OCC_COLOUR_DATA (OCC_PACKET_BYTES * 4U)
#define OCC_COLOUR_SYMBOLS (OCC_COLOUR_SYNC + OCC_COLOUR_DATA + 6U)
#define OCC_COLOUR_DARK 4
typedef struct {
    uint64_t symbol_ns;
    uint8_t channels[4]; /* Distinct one-bit logical masks, NOT physical pins. */
} occ_colour_profile;
occ_result occ_colour_encode(const occ_colour_profile *profile,
    const uint8_t bytes[OCC_PACKET_BYTES], occ_wave *wave);
typedef struct {
    uint64_t period_ns, tolerance_ns, last_ns;
    uint8_t have_last, stage, count, window[OCC_COLOUR_SYNC];
    uint8_t bytes[OCC_PACKET_BYTES];
} occ_colour_receiver;
occ_result occ_colour_receiver_init(occ_colour_receiver *rx,
    uint64_t period_ns, uint64_t tolerance_ns);
/* Exactly one independently recovered symbol per timestamped optical period.
 * Clock/ROI acquisition is outside this interface. -1 = erasure, 4 = dark.
 * Any gap, bad closing/guard, classification error or CRC aborts partial state.
 * Caller pins board/session/sequence using occ_tracker before accepting status. */
int occ_colour_receiver_push(occ_colour_receiver *rx, uint64_t time_ns,
    int symbol, occ_packet *packet);
typedef struct {
    uint16_t dark[3], prototypes[4][3]; /* prototypes are NET light above dark */
    uint16_t min_signal, saturation, max_distance, min_margin;
} occ_colour_calibration;
/* RGB values/thresholds must share one bounded scale (e.g. 0..255).
 * Hue distance is L1 after normalizing to total4096. Saturation and uncertain
 * colours are rejected, never guessed. Calibration is source/ROI-specific;
 * these operations do not establish authenticity or physical channel identity. */
occ_result occ_colour_calibration_check(const occ_colour_calibration *cal);
occ_result occ_colour_classify(const occ_colour_calibration *cal,
    const uint16_t rgb[3], int *symbol);
#ifdef __cplusplus
}
#endif
#endif
