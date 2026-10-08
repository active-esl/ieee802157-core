/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/rolling.h"
#include "occ/spatial.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
static uint32_t state=0x12345678U;
static uint32_t random32(void)
{
    state^=state<<13U; state^=state>>17U; state^=state<<5U; return state;
}
int main(void)
{
    uint8_t bytes[OCC_PACKET_BYTES],pixels[64],fragment[3];
    occ_packet packet;
    occ_receiver rx[3];
    occ_cook_receiver cook;
    occ_cook_assembler a;
    occ_gray_image image={pixels,8U,8U,8U};
    occ_roi roi;
    const occ_thresholds thresholds={70U,180U,100U,150U,2U,253U,250U};
    const occ_rolling_profile profile={227272U,40000U,10000U,3U};
    uint64_t stamp=0U;
    unsigned i,j,uncertain;
    int sample;
    for(j=0U;j<3U;++j) occ_receiver_init(&rx[j],(occ_rx_mode)j,1000U,100U);
    occ_cook_receiver_init(&cook,1000U); occ_cook_assembler_init(&a,100000U);
    for(i=0U;i<100000U;++i) {
        for(j=0U;j<OCC_PACKET_BYTES;++j) bytes[j]=(uint8_t)random32();
        /* Goal is memory/UB and argument handling, not proving authentication. */
        (void)occ_packet_decode(bytes,&packet);
        stamp+=(uint64_t)(random32()%2000U);
        for(j=0U;j<3U;++j)
            (void)occ_receiver_push(&rx[j],stamp,(int)(random32()%7U)-2,&packet);
        (void)occ_cook_receiver_push(&cook,stamp,(int)(random32()%5U)-1,fragment);
        if(i<1000U) {
            for(j=0U;j<64U;++j) pixels[j]=(uint8_t)random32();
            roi.x=random32()%16U; roi.y=random32()%16U;
            roi.width=random32()%16U; roi.height=random32()%16U;
            if(i%10U==0U) roi.x=SIZE_MAX;
            (void)occ_gray_classify(&image,&roi,&thresholds,&sample);
            (void)occ_cook_decode_image(&a,&image,&roi,&thresholds,&profile,
                UINT64_MAX-100U,UINT64_MAX-100U,&packet,&uncertain);
        }
    }
    assert(occ_receiver_init(NULL,OCC_RX_UFSOOK,1000U,1U)==OCC_INVALID);
    assert(occ_packet_decode(NULL,&packet)==OCC_INVALID);
    assert(occ_gray_classify(NULL,&roi,&thresholds,&sample)==OCC_INVALID);
    puts("PASS: 100000 deterministic malformed-input iterations and overflow/null/ROI boundaries");
    return 0;
}
