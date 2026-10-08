/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/player.h"
occ_result occ_player_init(occ_player *p,const occ_wave *w,
                          const occ_led_capabilities *c,uint64_t start,
                          uint64_t lateness)
{
    occ_player zero={0};
    uint64_t total,half,min_interval;
    size_t i;
    if(p==NULL || c==NULL || w==NULL || w->count==0U ||
       c->logical_channels==0U || c->min_interval_ns==0U ||
       occ_wave_duration(w,&total)!=OCC_OK || total>OCC_MAX_SEGMENT_NS ||
       UINT64_MAX-start<total) return OCC_INVALID;
    if(c->jitter_ns>lateness) return OCC_UNSUPPORTED;
    for(i=0U;i<w->count;++i) {
        const occ_segment *s=&w->segments[i];
        if((s->active_mask & c->logical_channels)!=s->active_mask)
            return OCC_UNSUPPORTED;
        min_interval=s->duration_ns;
        if(s->rate_num!=0U) {
            half=(UINT64_C(500000000)*s->rate_den)/s->rate_num;
            /* Include carrier edges and the residual interval at a segment
             * boundary, not just the nominal half-period. */
            if(half==0U) return OCC_UNSUPPORTED;
            if(half<min_interval) min_interval=half;
            /* Boundary-to-previous-carrier-edge may be shorter than half.
             * Include that tail to reject unexpectedly tiny schedule intervals. */
            {
                uint64_t h=(s->duration_ns*s->rate_num)/
                    (UINT64_C(500000000)*s->rate_den);
                uint64_t edge=(h*(UINT64_C(500000000)*s->rate_den)+
                               s->rate_num-1U)/s->rate_num;
                uint64_t tail=s->duration_ns-edge;
                if(tail!=0U && tail<min_interval) min_interval=tail;
            }
        }
        if(min_interval<c->min_interval_ns || lateness>=min_interval/2U)
            return OCC_UNSUPPORTED;
    }
    *p=zero; p->wave=w; p->start_ns=start; p->next_deadline_ns=start;
    p->max_lateness_ns=lateness;
    return OCC_OK;
}
occ_result occ_player_step(occ_player *p,uint64_t now,uint8_t *channels,
                          uint64_t *deadline)
{
    const occ_segment *s;
    uint64_t t,local,next,h,threshold;
    if(p==NULL || p->wave==NULL || channels==NULL || deadline==NULL)
        return OCC_INVALID;
    *channels=0U; *deadline=UINT64_MAX;
    if(p->aborted) return OCC_UNCERTAIN;
    if(p->finished) return OCC_OK;
    if(now<p->start_ns) { *deadline=p->start_ns; return OCC_OK; }
    if(now<p->next_deadline_ns) return OCC_INVALID;
    if(now-p->next_deadline_ns>p->max_lateness_ns) {
        p->aborted=1U; return OCC_UNCERTAIN;
    }
    t=now-p->start_ns;
    while(p->index<p->wave->count &&
          t-p->offset_ns>=p->wave->segments[p->index].duration_ns) {
        p->offset_ns+=p->wave->segments[p->index].duration_ns; p->index++;
    }
    if(p->index==p->wave->count) { p->finished=1U; return OCC_OK; }
    s=&p->wave->segments[p->index]; local=t-p->offset_ns; next=s->duration_ns;
    if(occ_segment_level(s,local,channels)!=OCC_OK) {
        p->aborted=1U; return OCC_INVALID;
    }
    if(s->rate_num!=0U) {
        threshold=UINT64_C(500000000)*s->rate_den;
        h=(local*s->rate_num)/threshold;
        {
            uint64_t edge=((h+1U)*threshold+s->rate_num-1U)/s->rate_num;
            if(edge<next) next=edge;
        }
    }
    p->next_deadline_ns=p->start_ns+p->offset_ns+next;
    *deadline=p->next_deadline_ns;
    return OCC_OK;
}
