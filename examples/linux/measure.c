/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "occ/rolling.h"
#include "occ/spatial.h"
#include "occ/player.h"
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
static uint64_t ns(void)
{
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)!=0) return 0U;
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
int main(void)
{
    occ_packet source={2U,42U,99U,7U,0U,1234U},decoded;
    uint8_t bytes[OCC_PACKET_BYTES],bits[OCC_PACKET_BITS];
    occ_segment segments[OCC_PACKET_BITS*2U+8U];
    occ_wave wave={segments,0U,OCC_PACKET_BITS*2U+8U};
    occ_ufsook_profile u={30U,1U,4U,1200U,1U};
    uint64_t begin,packet_ns,encode_ns;
    unsigned i;
    begin=ns();
    for(i=0U;i<50000U;++i) {
        source.sequence=(uint16_t)i;
        if(occ_packet_encode(&source,bytes)!=OCC_OK ||
           occ_packet_decode(bytes,&decoded)!=OCC_OK) return 1;
    }
    packet_ns=ns()-begin;
    occ_bytes_to_bits(bytes,sizeof(bytes),bits,sizeof(bits));
    begin=ns();
    for(i=0U;i<10000U;++i)
        if(occ_ufsook_encode(&u,bits,sizeof(bits),&wave)!=OCC_OK) return 1;
    encode_ns=ns()-begin;
    printf("{\"abi\":\"host\",\"segment_bytes\":%zu,\"receiver_bytes\":%zu,"
           "\"tracker_bytes\":%zu,\"player_bytes\":%zu,\"cook_assembler_bytes\":%zu,"
           "\"cook_receiver_bytes\":%zu,\"symbol_group_bytes\":%zu,"
           "\"ufsook_wave_buffer_bytes\":%zu,\"s2psk_wave_buffer_bytes\":%zu,"
           "\"cook_fragment_wave_buffer_bytes\":%zu,"
           "\"packet_encode_decode_mean_ns\":%" PRIu64 ",\"ufsook_encode_mean_ns\":%" PRIu64 "}\n",
        sizeof(occ_segment),sizeof(occ_receiver),sizeof(occ_tracker),sizeof(occ_player),
        sizeof(occ_cook_assembler),sizeof(occ_cook_receiver),sizeof(occ_symbol_group),
        (OCC_PACKET_BITS+4U)*sizeof(occ_segment),
        (OCC_PACKET_BITS*2U+8U)*sizeof(occ_segment),116U*sizeof(occ_segment),
        packet_ns/50000U,encode_ns/10000U);
    return 0;
}
