/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_PACKET_H
#define OCC_PACKET_H
#include "occ.h"
#ifdef __cplusplus
extern "C" {
#endif
#define OCC_PACKET_BYTES 22U
#define OCC_PACKET_BITS (OCC_PACKET_BYTES * 8U)
/* Custom diagnostic application envelope, NOT an IEEE MAC frame.
 * Explicit little-endian fields; byte bits serialized LSB first.
 * Kind: 1 boot, 2 heartbeat, 3 identity, 4 fault, 5 test result.
 * Identifiers are non-secret labels; no variable diagnostic blob. */
typedef struct {
    uint8_t kind;
    uint32_t board;
    uint32_t session;
    uint16_t sequence;
    uint16_t status;
    uint32_t build;
} occ_packet;
occ_result occ_packet_encode(const occ_packet *p, uint8_t out[OCC_PACKET_BYTES]);
occ_result occ_packet_decode(const uint8_t in[OCC_PACKET_BYTES], occ_packet *p);
occ_result occ_bytes_to_bits(const uint8_t *bytes, size_t n,
                            uint8_t *bits, size_t capacity);
occ_result occ_bits_to_bytes(const uint8_t *bits, size_t n,
                            uint8_t *bytes, size_t capacity);
/* Tracker is pinned to one expected board/source ROI. A changed session is
 * refused until the application explicitly reinitializes this tracker.
 * This prevents implicit identity reset, NOT malicious replay/spoofing.
 * Identical repeats retain signal but do not refresh last_progress_ns.
 * Changed contents at the same sequence are rejected without state updates. */
typedef struct {
    uint32_t expected_board;
    uint32_t session;
    uint16_t sequence;
    uint16_t status;
    uint32_t build;
    uint8_t kind;
    uint8_t have_session;
    occ_observation observation;
} occ_tracker;
void occ_tracker_init(occ_tracker *t, uint32_t expected_board);
occ_result occ_tracker_accept(occ_tracker *t, uint64_t now_ns,
                             const occ_packet *p, int *progresses);
#ifdef __cplusplus
}
#endif
#endif
