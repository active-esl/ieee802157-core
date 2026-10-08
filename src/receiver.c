/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/receiver.h"
static void restart(occ_receiver *r)
{
    unsigned i;
    r->stage=0U; r->have_pair=0U; r->window_count=0U; r->data_count=0U;
    for(i=0U;i<OCC_PACKET_BYTES;++i) r->bytes[i]=0U;
}
occ_result occ_receiver_init(occ_receiver *r, occ_rx_mode mode,
                            uint64_t period, uint64_t tolerance)
{
    occ_receiver zero={0};
    if(r==NULL || mode<OCC_RX_UFSOOK || mode>OCC_RX_COOK ||
       period==0U || period>OCC_MAX_SEGMENT_NS || tolerance>=period/2U)
        return OCC_INVALID;
    *r=zero; r->mode=mode; r->period_ns=period; r->tolerance_ns=tolerance;
    return OCC_OK;
}
static void window_push(occ_receiver *r, uint8_t s, unsigned n)
{
    unsigned i;
    if(r->window_count<n) r->window[r->window_count++]=s;
    else {
        for(i=1U;i<n;++i) r->window[i-1U]=r->window[i];
        r->window[n-1U]=s;
    }
}
static int delimiter(const occ_receiver *r)
{
    static const uint8_t cook[6]={0U,1U,1U,1U,0U,0U};
    unsigned i;
    if(r->mode==OCC_RX_COOK) {
        if(r->window_count<6U) return 0;
        for(i=0U;i<6U;++i) if(r->window[i]!=cook[i]) return 0;
        return 1;
    }
    if(r->window_count<4U) return 0;
    if(r->mode==OCC_RX_S2PSK) {
        for(i=0U;i<4U;++i) if(r->window[i]!=1U) return 0;
        return 1;
    }
    return r->window[0]==OCC_SAMPLE_MID &&
        r->window[1]==OCC_SAMPLE_MID && r->window[2]<=1U &&
        r->window[3]<=1U && r->window[2]!=r->window[3];
}
static int finish(occ_receiver *r, occ_packet *packet)
{
    occ_result result=occ_packet_decode(r->bytes,packet);
    restart(r);
    return result==OCC_OK ? OCC_PACKET_READY : OCC_UNCERTAIN;
}
int occ_receiver_push(occ_receiver *r, uint64_t time, int sample,
                      occ_packet *packet)
{
    uint64_t delta;
    uint8_t bit;
    occ_result result;
    unsigned window_size;
    if(r==NULL || packet==NULL || r->period_ns==0U ||
       r->mode<OCC_RX_UFSOOK || r->mode>OCC_RX_COOK) return OCC_INVALID;
    if(r->have_last) {
        if(time<=r->last_ns) { restart(r); return OCC_UNCERTAIN; }
        delta=time-r->last_ns;
        if(delta<r->period_ns-r->tolerance_ns ||
           delta>r->period_ns+r->tolerance_ns) {
            r->last_ns=time; restart(r); return OCC_UNCERTAIN;
        }
    }
    r->have_last=1U; r->last_ns=time;
    if(sample<0 || sample>OCC_SAMPLE_MID ||
       (sample==OCC_SAMPLE_MID && r->mode!=OCC_RX_UFSOOK)) {
        restart(r); return OCC_UNCERTAIN;
    }
    window_size=r->mode==OCC_RX_COOK ? 6U : 4U;
    if(r->stage==0U) {
        window_push(r,(uint8_t)sample,window_size);
        if(delimiter(r)) { r->stage=1U; r->window_count=0U; }
        return OCC_OK;
    }
    if(r->stage==2U) {
        window_push(r,(uint8_t)sample,window_size);
        if(r->window_count==window_size) {
            if(!delimiter(r)) { restart(r); return OCC_UNCERTAIN; }
            return finish(r,packet);
        }
        return OCC_OK;
    }
    if(sample>1) { restart(r); return OCC_UNCERTAIN; }
    if(!r->have_pair) { r->pair_first=(uint8_t)sample; r->have_pair=1U; return OCC_OK; }
    r->have_pair=0U;
    if(r->mode==OCC_RX_UFSOOK)
        result=occ_ufsook_demod_pair(r->pair_first,sample,&bit);
    else if(r->mode==OCC_RX_S2PSK)
        result=occ_s2psk_line_decode(r->pair_first,(uint8_t)sample,&bit);
    else result=occ_manchester_decode(r->pair_first,(uint8_t)sample,&bit);
    if(result!=OCC_OK) { restart(r); return OCC_UNCERTAIN; }
    if(r->mode==OCC_RX_COOK) {
        if(r->data_count==0U) { r->ab=bit; r->data_count++; return OCC_OK; }
        if(r->data_count==OCC_PACKET_BITS+1U) {
            if(bit!=r->ab) { restart(r); return OCC_UNCERTAIN; }
            return finish(r,packet);
        }
        r->bytes[(r->data_count-1U)/8U] |=
            (uint8_t)(bit<<((r->data_count-1U)%8U));
        r->data_count++;
    } else {
        r->bytes[r->data_count/8U] |= (uint8_t)(bit<<(r->data_count%8U));
        if(++r->data_count==OCC_PACKET_BITS) { r->stage=2U; r->window_count=0U; }
    }
    return OCC_OK;
}
