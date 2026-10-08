/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/image.h"
occ_result occ_gray_classify(const occ_gray_image *im, const occ_roi *roi,
                            const occ_thresholds *p, int *sample)
{
    size_t x,y,pixels;
    uint64_t sum=0U,clipped=0U;
    uint32_t mean;
    if(im==NULL || roi==NULL || p==NULL || sample==NULL || im->data==NULL ||
       im->width==0U || im->height==0U || im->stride<im->width ||
       im->height>SIZE_MAX/im->stride ||
       roi->width==0U || roi->height==0U ||
       roi->x>im->width || roi->y>im->height ||
       roi->width>im->width-roi->x || roi->height>im->height-roi->y ||
       roi->width>SIZE_MAX/roi->height ||
       p->low_max>=p->mid_min || p->mid_min>p->mid_max ||
       p->mid_max>=p->high_min || p->clipped_low>=p->clipped_high ||
       p->max_clipped_per_mille>1000U) return OCC_INVALID;
    pixels=roi->width*roi->height;
    if(pixels>UINT64_MAX/1000U) return OCC_INVALID;
    for(y=0U;y<roi->height;++y) for(x=0U;x<roi->width;++x) {
        uint8_t v=im->data[(roi->y+y)*im->stride+roi->x+x];
        sum+=v;
        if(v<=p->clipped_low || v>=p->clipped_high) clipped++;
    }
    if(clipped*1000U>(uint64_t)pixels*p->max_clipped_per_mille) {
        *sample=OCC_SAMPLE_ERASURE; return OCC_OK;
    }
    mean=(uint32_t)(sum/pixels);
    if(mean<=p->low_max) *sample=0;
    else if(mean>=p->high_min) *sample=1;
    else if(mean>=p->mid_min && mean<=p->mid_max) *sample=OCC_SAMPLE_MID;
    else *sample=OCC_SAMPLE_ERASURE;
    return OCC_OK;
}
