/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/spatial.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t pixels[16];
    occ_gray_image im={pixels,4U,4U,4U};
    occ_roi a={0U,0U,2U,2U},b={2U,0U,2U,2U};
    const occ_thresholds p={70U,180U,100U,150U,2U,253U,250U};
    occ_symbol_group g;
    uint64_t stamp;
    int sample;
    unsigned i;
    memset(pixels,220,sizeof(pixels));
    assert(occ_s2psk_image_sample(&im,&a,&b,&p,&sample)==OCC_OK && sample==1);
    for(i=0U;i<2U;++i) { pixels[4U*i+2U]=20U; pixels[4U*i+3U]=20U; }
    assert(occ_s2psk_image_sample(&im,&a,&b,&p,&sample)==OCC_OK && sample==0);
    b.x=1U;
    assert(occ_s2psk_image_sample(&im,&a,&b,&p,&sample)==OCC_UNSUPPORTED);
    assert(occ_symbol_group_init(&g,0U,100U,70U,2U)==OCC_OK);
    assert(occ_symbol_group_push(&g,10U,1,&stamp,&sample)==OCC_OK);
    assert(occ_symbol_group_push(&g,40U,1,&stamp,&sample)==OCC_OK);
    assert(occ_symbol_group_push(&g,110U,0,&stamp,&sample)==OCC_SYMBOL_READY && sample==1 && stamp==0U);
    assert(occ_symbol_group_push(&g,140U,1,&stamp,&sample)==OCC_OK);
    assert(occ_symbol_group_push(&g,210U,0,&stamp,&sample)==OCC_SYMBOL_READY && sample==3);
    assert(occ_symbol_group_flush(&g,250U,&stamp,&sample)==OCC_UNCERTAIN);
    assert(occ_symbol_group_flush(&g,300U,&stamp,&sample)==OCC_SYMBOL_READY && sample==3);
    puts("PASS: Figure179 spatial polarity, disjoint ROIs, oversampling, inconsistent/partial-symbol rejection");
    return 0;
}
