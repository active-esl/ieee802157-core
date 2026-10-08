/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/occ.h"
#include <inttypes.h>
#include <stdio.h>
int main(void)
{
    occ_segment segments[16];
    occ_wave w={segments,0U,16U};
    occ_ufsook_profile p={30U,1U,4U,1200U,1U};
    uint8_t bits[]={0U,1U,1U,0U}, level;
    uint64_t t,total;
    if(occ_ufsook_encode(&p,bits,4U,&w)!=OCC_OK ||
       occ_wave_duration(&w,&total)!=OCC_OK) return 1;
    puts("time_ns,logical_channels");
    for(t=0U;t<total;t+=100000U) {
        if(occ_wave_level(&w,t,&level)!=OCC_OK) return 1;
        printf("%" PRIu64 ",%u\n",t,(unsigned)level);
    }
    return 0;
}
