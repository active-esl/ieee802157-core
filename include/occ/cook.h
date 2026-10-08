/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_COOK_H
#define OCC_COOK_H
#include "packet.h"
#ifdef __cplusplus
extern "C" {
#endif
#define OCC_COOK_FRAGMENT_BYTES 3U
#define OCC_COOK_FRAGMENT_BITS 24U
/* Custom application fragment: tag=(generation3bits<<5)|byte_index5bits,
 * data octet, CRC-8/SMBUS (poly07, init0, no reflection/xorout).
 * One checked envelope octet per fragment, NOT an IEEE MAC/FEC mechanism. */
uint8_t occ_crc8(const uint8_t *bytes,size_t n);
occ_result occ_cook_fragment_encode(const uint8_t packet[OCC_PACKET_BYTES],
             uint8_t generation,uint8_t index,uint8_t fragment[3]);
typedef struct {
    uint8_t bytes[OCC_PACKET_BYTES],next,generation,active;
    uint8_t last_tag,last_data,have_last;
    uint64_t last_ns,timeout_ns;
} occ_cook_assembler;
occ_result occ_cook_assembler_init(occ_cook_assembler *a,uint64_t timeout_ns);
/* Complete packet returns1; gaps, reordered/conflicting fragments, corruption,
 * timeout or mixed generation reject without emitting packet. Exact repeated
 * fragment is ignored and does not refresh progress. Final CRC16 still gates. */
int occ_cook_fragment_accept(occ_cook_assembler *a,uint64_t now_ns,
                             const uint8_t fragment[3],occ_packet *packet);

/* Manchester C-OOK raw3-octet sub-packet, matching start/end Ab.
 * Recovered optical-clock samples only. Mid/erasure/gaps reset partial state.
 * No inner/outer FEC, partial-subpacket fusion or blind clock recovery. */
typedef struct {
    uint64_t period_ns,last_ns;
    uint8_t have_last,stage,window,count,first,have_pair,ab;
    uint8_t bytes[3];
} occ_cook_receiver;
occ_result occ_cook_receiver_init(occ_cook_receiver *r,uint64_t period_ns);
int occ_cook_receiver_push(occ_cook_receiver *r,uint64_t symbol_ns,int sample,
                            uint8_t fragment[3]);
#ifdef __cplusplus
}
#endif
#endif
