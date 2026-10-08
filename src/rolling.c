/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/rolling.h"
static int recovered(occ_cook_receiver *r,occ_cook_assembler *a,
                      uint64_t stamp,int value,occ_packet *packet)
{
    uint8_t f[3];
    int result=occ_cook_receiver_push(r,stamp,value,f);
    if(result==1) return occ_cook_fragment_accept(a,stamp,f,packet);
    return result;
}
int occ_cook_decode_image(occ_cook_assembler *a,
        const occ_gray_image *im,const occ_roi *source,
        const occ_thresholds *thresholds,const occ_rolling_profile *p,
        uint64_t first,uint64_t epoch,occ_packet *packet,unsigned *uncertain)
{
    occ_cook_receiver r;
    occ_roi row;
    uint64_t cell=0U,newcell,time;
    size_t y;
    unsigned count=0U;
    int decision=0,bad=0,sample,accepted=0,result;
    if(a==NULL || im==NULL || source==NULL || thresholds==NULL || p==NULL ||
       packet==NULL || uncertain==NULL || p->row_period_ns==0U ||
       p->exposure_ns==0U || p->exposure_ns>OCC_MAX_SEGMENT_NS ||
       p->min_confident_rows<2U || p->min_confident_rows>16U ||
       p->row_period_ns>OCC_MAX_SEGMENT_NS || epoch>first ||
       first>UINT64_MAX-p->exposure_ns/2U ||
       source->height==0U || source->y>im->height ||
       source->height>im->height-source->y ||
       source->height>(UINT64_MAX-first-p->exposure_ns/2U)/p->row_period_ns ||
       occ_cook_receiver_init(&r,p->optical_period_ns)!=OCC_OK) return OCC_INVALID;
    row=*source; row.height=1U; *uncertain=0U;
    for(y=0U;y<source->height;++y) {
        row.y=source->y+y;
        if(occ_gray_classify(im,&row,thresholds,&sample)!=OCC_OK) return OCC_INVALID;
        time=first+(uint64_t)y*p->row_period_ns+p->exposure_ns/2U;
        newcell=(time-epoch)/p->optical_period_ns;
        if(y!=0U && newcell!=cell) {
            result=recovered(&r,a,epoch+cell*p->optical_period_ns,
                bad || count<p->min_confident_rows ? OCC_SAMPLE_ERASURE : decision,packet);
            if(result==1) accepted=1;
            else if(result<0) (*uncertain)++;
            count=0U; bad=0;
        }
        cell=newcell;
        if(sample==0 || sample==1) {
            if(count!=0U && decision!=sample) bad=1;
            decision=sample;
            if(count<UINT16_MAX) count++;
        }
    }
    /* Deliberately ignore the final, potentially incomplete cell. */
    return accepted;
}
