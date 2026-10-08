/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/packet.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv)
{
    occ_segment segments[OCC_PACKET_BITS*2U+8U];
    occ_wave w={segments,0U,OCC_PACKET_BITS*2U+8U};
    occ_ufsook_profile profile={30U,1U,4U,1200U,1U};
    occ_s2psk_profile spatial={10U,1000U,1U,2U};
    occ_packet p={2U,0x12345678U,0x10203040U,7U,3U,0x87654321U};
    uint8_t bytes[OCC_PACKET_BYTES],bits[OCC_PACKET_BITS];
    size_t i;
    if(occ_packet_encode(&p,bytes)!=OCC_OK ||
       occ_bytes_to_bits(bytes,sizeof(bytes),bits,sizeof(bits))!=OCC_OK) return 1;
    if(argc==1) {
        if(occ_ufsook_encode(&profile,bits,sizeof(bits),&w)!=OCC_OK) return 1;
    } else if(argc==2 && strcmp(argv[1],"s2psk")==0) {
        if(occ_s2psk_encode(&spatial,bits,sizeof(bits),&w)!=OCC_OK) return 1;
    } else return 1;
    printf("{\"board\":%" PRIu32 ",\"sequence\":%u,\"segments\":[",p.board,(unsigned)p.sequence);
    for(i=0U;i<w.count;++i) {
        const occ_segment *s=&segments[i];
        printf("%s{\"duration_ns\":%" PRIu64 ",\"rate_num\":%" PRIu32 ",\"rate_den\":%" PRIu32 ",\"active\":%u,\"inverted\":%u}",
            i==0U?"":",",s->duration_ns,s->rate_num,s->rate_den,
            (unsigned)s->active_mask,(unsigned)s->inverted_mask);
    }
    puts("]}"); return 0;
}
