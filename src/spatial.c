/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/spatial.h"
occ_result occ_s2psk_image_sample(const occ_gray_image *im,
                                 const occ_roi *a,const occ_roi *b,
                                 const occ_thresholds *p,int *sample)
{
    int sa,sb;
    uint8_t bit;
    occ_result r;
    if(a==NULL || b==NULL || sample==NULL) return OCC_INVALID;
    r=occ_gray_classify(im,a,p,&sa); if(r!=OCC_OK) return r;
    r=occ_gray_classify(im,b,p,&sb); if(r!=OCC_OK) return r;
    /* Classifier has checked bounds, so these sums cannot overflow. */
    if(a->x<b->x+b->width && b->x<a->x+a->width &&
       a->y<b->y+b->height && b->y<a->y+a->height) return OCC_UNSUPPORTED;
    if(occ_s2psk_demod_pair(sa,sb,&bit)!=OCC_OK) *sample=OCC_SAMPLE_ERASURE;
    else *sample=bit;
    return OCC_OK;
}
occ_result occ_symbol_group_init(occ_symbol_group *g,uint64_t epoch,
                                 uint64_t period,uint64_t gap,uint16_t minimum)
{
    occ_symbol_group zero={0};
    if(g==NULL || period==0U || period>OCC_MAX_SEGMENT_NS || gap==0U ||
       gap>=period || minimum<2U || minimum>16U) return OCC_INVALID;
    *g=zero; g->epoch_ns=epoch; g->period_ns=period;
    g->max_capture_gap_ns=gap; g->min_samples=minimum; return OCC_OK;
}
static void output(const occ_symbol_group *g,uint64_t *stamp,int *sample)
{
    *stamp=g->epoch_ns+g->slot*g->period_ns;
    *sample=g->bad || g->count<g->min_samples ? OCC_SAMPLE_ERASURE : g->decision;
}
static void add(occ_symbol_group *g,int sample)
{
    if(sample!=0 && sample!=1) { g->bad=1U; return; }
    if(g->count!=0U && g->decision!=(uint8_t)sample) g->bad=1U;
    g->decision=(uint8_t)sample;
    if(g->count<UINT16_MAX) g->count++;
}
int occ_symbol_group_push(occ_symbol_group *g,uint64_t time,int sample,
                           uint64_t *stamp,int *symbol)
{
    uint64_t slot;
    int ready=OCC_OK,bad_gap=0;
    if(g==NULL || stamp==NULL || symbol==NULL || g->period_ns==0U ||
       time<g->epoch_ns) return OCC_INVALID;
    slot=(time-g->epoch_ns)/g->period_ns;
    if(g->have_slot) {
        if(time<=g->last_capture_ns) { g->bad=1U; return OCC_UNCERTAIN; }
        if(time-g->last_capture_ns>g->max_capture_gap_ns) {
            g->bad=1U; bad_gap=1;
        }
        if(slot!=g->slot) {
            output(g,stamp,symbol); ready=OCC_SYMBOL_READY;
            g->count=0U; g->bad=(uint8_t)bad_gap;
        }
    } else g->have_slot=1U;
    g->slot=slot; g->last_capture_ns=time; add(g,sample);
    return ready;
}
int occ_symbol_group_flush(occ_symbol_group *g,uint64_t end,
                            uint64_t *stamp,int *symbol)
{
    uint64_t start;
    if(g==NULL || stamp==NULL || symbol==NULL || !g->have_slot)
        return OCC_INVALID;
    start=g->epoch_ns+g->slot*g->period_ns;
    if(UINT64_MAX-start<g->period_ns || end<start+g->period_ns)
        return OCC_UNCERTAIN;
    output(g,stamp,symbol); g->have_slot=0U;
    return OCC_SYMBOL_READY;
}
