/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/cook.h"
#include <inttypes.h>
#include <stdio.h>
int main(void)
{
    occ_packet p={2U,0x12345678U,0x10203040U,7U,3U,0x87654321U};
    uint8_t packet[OCC_PACKET_BYTES],fragment[3],bits[24];
    occ_segment segments[116];
    occ_wave w={segments,0U,116U};
    occ_cook_profile profile={4400U,2U,1U};
    unsigned index; size_t j;
    if(occ_packet_encode(&p,packet)!=OCC_OK) return 1;
    fputs("{\"fragments\":[",stdout);
    for(index=0U;index<OCC_PACKET_BYTES;++index) {
        if(occ_cook_fragment_encode(packet,1U,(uint8_t)index,fragment)!=OCC_OK ||
           occ_bytes_to_bits(fragment,3U,bits,sizeof(bits))!=OCC_OK ||
           occ_cook_encode(&profile,(uint8_t)(index&1U),bits,sizeof(bits),&w)!=OCC_OK)
            return 1;
        printf("%s{\"segments\":[",index==0U?"":",");
        for(j=0U;j<w.count;++j) {
            const occ_segment *s=&segments[j];
            printf("%s{\"duration_ns\":%" PRIu64 ",\"rate_num\":%" PRIu32 ",\"rate_den\":%" PRIu32 ",\"active\":%u,\"inverted\":%u}",
                j==0U?"":",",s->duration_ns,s->rate_num,s->rate_den,
                (unsigned)s->active_mask,(unsigned)s->inverted_mask);
        }
        fputs("]}",stdout);
    }
    puts("]}"); return 0;
}
