/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/colour.h"
#include <string.h>
static const uint8_t sync_word[OCC_COLOUR_SYNC] =
    {0,1,0,2,3,1,2,0,3,2,1,3,0,2,1,0};
static const uint8_t trailer[4] = {3,1,3,2};
static int one_bit(uint8_t x) { return x && !(x & (uint8_t)(x-1U)); }
occ_result occ_colour_encode(const occ_colour_profile *p,
    const uint8_t bytes[OCC_PACKET_BYTES], occ_wave *w)
{
    occ_packet ignored;
    size_t i;
    uint8_t used=0;
    if (!p || !bytes || !w || !w->segments || !p->symbol_ns ||
        p->symbol_ns>OCC_MAX_SEGMENT_NS) return OCC_INVALID;
    for (i=0;i<4U;i++) {
        if (!one_bit(p->channels[i]) || (used & p->channels[i])) return OCC_INVALID;
        used |= p->channels[i];
    }
    if (occ_packet_decode(bytes,&ignored)!=OCC_OK) return OCC_INVALID;
    if (w->capacity<OCC_COLOUR_SYMBOLS) return OCC_CAPACITY;
    for (i=0;i<OCC_COLOUR_SYMBOLS;i++) {
        unsigned colour;
        occ_segment s={p->symbol_ns,0,1,0,0};
        if (i<OCC_COLOUR_SYNC) colour=sync_word[i];
        else if (i<OCC_COLOUR_SYNC+OCC_COLOUR_DATA) {
            size_t n=i-OCC_COLOUR_SYNC;
            colour=(unsigned)(bytes[n/4U] >> (2U*(n%4U))) & 3U;
        } else if (i<OCC_COLOUR_SYNC+OCC_COLOUR_DATA+4U)
            colour=trailer[i-OCC_COLOUR_SYNC-OCC_COLOUR_DATA];
        else colour=4U;
        if (colour<4U) s.active_mask=p->channels[colour];
        w->segments[i]=s;
    }
    w->count=OCC_COLOUR_SYMBOLS;
    return OCC_OK;
}
static void restart(occ_colour_receiver *r)
{
    r->stage=0; r->count=0;
    memset(r->bytes,0,sizeof(r->bytes));
}
occ_result occ_colour_receiver_init(occ_colour_receiver *r,
    uint64_t period, uint64_t tolerance)
{
    occ_colour_receiver zero={0};
    if (!r || !period || period>OCC_MAX_SEGMENT_NS || tolerance>=period/2U)
        return OCC_INVALID;
    *r=zero; r->period_ns=period; r->tolerance_ns=tolerance;
    return OCC_OK;
}
int occ_colour_receiver_push(occ_colour_receiver *r,uint64_t t,
    int symbol,occ_packet *packet)
{
    uint64_t delta;
    if (!r || !packet || !r->period_ns || r->stage>3U) return OCC_INVALID;
    if (r->have_last) {
        if (t<=r->last_ns) { restart(r); return OCC_UNCERTAIN; }
        delta=t-r->last_ns;
        if (delta<r->period_ns-r->tolerance_ns ||
            delta>r->period_ns+r->tolerance_ns) {
            r->last_ns=t; restart(r); return OCC_UNCERTAIN;
        }
    }
    r->have_last=1; r->last_ns=t;
    if (symbol<0 || symbol>OCC_COLOUR_DARK) { restart(r); return OCC_UNCERTAIN; }
    if (r->stage==0U) {
        if (symbol==OCC_COLOUR_DARK) { restart(r); return OCC_OK; }
        if (r->count<OCC_COLOUR_SYNC) r->window[r->count++]=(uint8_t)symbol;
        else {
            memmove(r->window,r->window+1,OCC_COLOUR_SYNC-1U);
            r->window[OCC_COLOUR_SYNC-1U]=(uint8_t)symbol;
        }
        if (r->count==OCC_COLOUR_SYNC &&
            !memcmp(r->window,sync_word,OCC_COLOUR_SYNC)) { r->stage=1; r->count=0; }
        return OCC_OK;
    }
    if (r->stage==1U) {
        if (symbol==OCC_COLOUR_DARK) { restart(r); return OCC_UNCERTAIN; }
        r->bytes[r->count/4U] |= (uint8_t)((unsigned)symbol << (2U*(r->count%4U)));
        if (++r->count==OCC_COLOUR_DATA) { r->stage=2; r->count=0; }
        return OCC_OK;
    }
    if (r->stage==2U) {
        if (symbol!=trailer[r->count]) { restart(r); return OCC_UNCERTAIN; }
        if (++r->count==4U) { r->stage=3; r->count=0; }
        return OCC_OK;
    }
    if (symbol!=OCC_COLOUR_DARK) { restart(r); return OCC_UNCERTAIN; }
    if (++r->count==2U) {
        occ_result rc=occ_packet_decode(r->bytes,packet);
        restart(r);
        return rc==OCC_OK ? OCC_PACKET_READY : OCC_UNCERTAIN;
    }
    return OCC_OK;
}
static void normalize(const uint16_t in[3],uint32_t out[3])
{
    unsigned i;
    uint32_t sum=(uint32_t)in[0]+in[1]+in[2];
    for (i=0;i<3U;i++) out[i]=sum ? ((uint32_t)in[i]*4096U)/sum : 0U;
}
static uint32_t distance(const uint32_t a[3],const uint32_t b[3])
{
    unsigned i; uint32_t d=0;
    for (i=0;i<3U;i++) d+=a[i]>b[i] ? a[i]-b[i] : b[i]-a[i];
    return d;
}
occ_result occ_colour_calibration_check(const occ_colour_calibration *c)
{
    uint32_t norm[4][3]; unsigned i,j,k;
    if (!c || !c->min_signal || !c->saturation || !c->max_distance ||
        c->max_distance>8192U || !c->min_margin || c->min_margin>8192U)
        return OCC_INVALID;
    for (k=0;k<3U;k++) if (c->dark[k]>=c->saturation) return OCC_INVALID;
    for (i=0;i<4U;i++) {
        uint32_t sum=0;
        for (k=0;k<3U;k++) {
            if ((uint32_t)c->prototypes[i][k]+c->dark[k]>=c->saturation)
                return OCC_INVALID;
            sum+=c->prototypes[i][k];
        }
        if (sum<c->min_signal) return OCC_INVALID;
        normalize(c->prototypes[i],norm[i]);
        for (j=0;j<i;j++) if (distance(norm[i],norm[j])<2U*c->min_margin)
            return OCC_UNCERTAIN;
    }
    return OCC_OK;
}
occ_result occ_colour_classify(const occ_colour_calibration *c,
    const uint16_t rgb[3],int *symbol)
{
    uint16_t net[3]; uint32_t norm[3],ref[3],sum=0,best=UINT32_MAX,second=UINT32_MAX;
    unsigned i,k,winner=0; occ_result rc;
    if (!rgb || !symbol) return OCC_INVALID;
    rc=occ_colour_calibration_check(c);
    if (rc!=OCC_OK) return rc;
    for (k=0;k<3U;k++) {
        if (rgb[k]>=c->saturation) return OCC_UNCERTAIN;
        net[k]=rgb[k]>c->dark[k] ? (uint16_t)(rgb[k]-c->dark[k]) : 0;
        sum+=net[k];
    }
    if (sum<c->min_signal) { *symbol=OCC_COLOUR_DARK; return OCC_OK; }
    normalize(net,norm);
    for (i=0;i<4U;i++) {
        uint32_t d;
        normalize(c->prototypes[i],ref); d=distance(norm,ref);
        if (d<best) { second=best; best=d; winner=i; }
        else if (d<second) second=d;
    }
    if (best>c->max_distance || second-best<c->min_margin) return OCC_UNCERTAIN;
    *symbol=(int)winner; return OCC_OK;
}
