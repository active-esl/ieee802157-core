/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/image.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t frame[16];
    occ_gray_image im={frame,4U,4U,4U};
    occ_roi roi={1U,1U,2U,2U};
    occ_thresholds p={70U,180U,100U,150U,2U,253U,250U};
    int sample;
    memset(frame,20,sizeof(frame));
    assert(occ_gray_classify(&im,&roi,&p,&sample)==OCC_OK && sample==0);
    memset(frame,220,sizeof(frame));
    assert(occ_gray_classify(&im,&roi,&p,&sample)==OCC_OK && sample==1);
    memset(frame,120,sizeof(frame));
    assert(occ_gray_classify(&im,&roi,&p,&sample)==OCC_OK && sample==2);
    memset(frame,160,sizeof(frame));
    assert(occ_gray_classify(&im,&roi,&p,&sample)==OCC_OK && sample==3);
    memset(frame,255,sizeof(frame));
    assert(occ_gray_classify(&im,&roi,&p,&sample)==OCC_OK && sample==3);
    roi.x=4U;
    assert(occ_gray_classify(&im,&roi,&p,&sample)==OCC_INVALID);
    puts("PASS: calibrated ROI levels, saturation/uncertainty, bounds");
    return 0;
}
