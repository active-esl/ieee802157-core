/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_RECEIVER_H
#define OCC_RECEIVER_H
#include "packet.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { OCC_RX_UFSOOK, OCC_RX_S2PSK, OCC_RX_COOK } occ_rx_mode;
#define OCC_PACKET_READY 1
#define OCC_SAMPLE_MID 2
#define OCC_SAMPLE_ERASURE 3
/* A framing receiver for the fixed application packet, not a generic MAC.
 * Input contract:
 * UFSOOK: one exposure-classified frame brightness per configured camera period.
 * S2-PSK: one confident Figure179 phase decision per recovered line-symbol period.
 * C-OOK: one confident brightness decision per recovered optical-clock period.
 * Pixel/row/exposure classification and clock recovery are outside this core.
 * A gap/erasure aborts partial reception; it is NEVER bridged silently.
 * Mid-level samples are usable only in the UFSOOK high-frequency delimiter.
 * All state and bounded packet buffer are caller owned. */
typedef struct {
    occ_rx_mode mode;
    uint64_t period_ns, tolerance_ns, last_ns;
    uint8_t have_last, stage, pair_first, have_pair, ab;
    uint8_t window[6], window_count;
    uint16_t data_count;
    uint8_t bytes[OCC_PACKET_BYTES];
} occ_receiver;
occ_result occ_receiver_init(occ_receiver *rx, occ_rx_mode mode,
                            uint64_t period_ns, uint64_t tolerance_ns);
/* Returns OCC_PACKET_READY only after full framing and packet integrity checks.
 * OCC_UNCERTAIN signals invalid timing/framing/integrity; output unchanged.
 * OCC_OK indicates no complete packet yet. */
int occ_receiver_push(occ_receiver *rx, uint64_t time_ns, int sample,
                      occ_packet *packet);
#ifdef __cplusplus
}
#endif
#endif
