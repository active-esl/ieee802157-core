/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/cook.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    occ_packet p={2U,42U,99U,7U,0U,1234U},out={0};
    uint8_t packet[OCC_PACKET_BYTES],f[3],corrupt[3],bits[24],decoded[3];
    occ_cook_assembler a;
    occ_cook_receiver r;
    occ_segment segments[58];
    occ_wave w={segments,0U,58U};
    occ_cook_profile profile={4400U,1U,1U};
    uint64_t now=0U;
    uint8_t level; unsigned index,i,j; int result=0;
    assert(occ_crc8((const uint8_t *)"123456789",9U)==0xf4U);
    assert(occ_packet_encode(&p,packet)==OCC_OK);
    assert(occ_cook_assembler_init(&a,100000U)==OCC_OK);
    for(index=0U;index<OCC_PACKET_BYTES;++index) {
        assert(occ_cook_fragment_encode(packet,1U,(uint8_t)index,f)==OCC_OK);
        result=occ_cook_fragment_accept(&a,now++,f,&out);
        assert(result==(index==OCC_PACKET_BYTES-1U ? 1 : OCC_OK));
        assert(occ_cook_fragment_accept(&a,now++,f,&out)==OCC_OK);
    }
    assert(out.board==p.board);
    for(i=0U;i<3U;++i) for(j=0U;j<8U;++j) {
        occ_cook_assembler_init(&a,100000U);
        occ_cook_fragment_encode(packet,1U,0U,f); memcpy(corrupt,f,3U);
        corrupt[i]^=(uint8_t)(1U<<j);
        assert(occ_cook_fragment_accept(&a,now++,corrupt,&out)==OCC_UNCERTAIN);
    }
    occ_cook_assembler_init(&a,100000U);
    occ_cook_fragment_encode(packet,1U,0U,f);
    assert(occ_cook_fragment_accept(&a,0U,f,&out)==OCC_OK);
    occ_cook_fragment_encode(packet,1U,2U,f);
    assert(occ_cook_fragment_accept(&a,1U,f,&out)==OCC_UNCERTAIN);
    occ_cook_assembler_init(&a,100U);
    occ_cook_fragment_encode(packet,1U,0U,f);
    assert(occ_cook_fragment_accept(&a,0U,f,&out)==OCC_OK);
    occ_cook_fragment_encode(packet,1U,1U,f);
    assert(occ_cook_fragment_accept(&a,101U,f,&out)==OCC_UNCERTAIN);
    occ_cook_fragment_encode(packet,1U,0U,f);
    occ_bytes_to_bits(f,3U,bits,24U);
    assert(occ_cook_encode(&profile,1U,bits,24U,&w)==OCC_OK && w.count==58U);
    occ_cook_receiver_init(&r,227272U);
    for(i=0U;i<58U;++i) {
        occ_segment_level(&segments[i],100U,&level);
        result=occ_cook_receiver_push(&r,(uint64_t)i*227272U,level,decoded);
    }
    assert(result==1 && memcmp(f,decoded,3U)==0);
    puts("PASS: CRC8 vector, 24 fragment single-bit corruptions, ordered/duplicate/timeout assembly, C-OOK subpacket receiver");
    return 0;
}
