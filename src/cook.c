/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/cook.h"
uint8_t occ_crc8(const uint8_t *bytes,size_t n)
{
    uint8_t crc=0U;
    size_t i; unsigned j;
    if(bytes==NULL && n!=0U) return 0U;
    for(i=0U;i<n;++i) {
        crc^=bytes[i];
        for(j=0U;j<8U;++j)
            crc=(uint8_t)((uint8_t)(crc<<1U) ^ ((crc&0x80U)?0x07U:0U));
    }
    return crc;
}
occ_result occ_cook_fragment_encode(const uint8_t packet[OCC_PACKET_BYTES],
             uint8_t generation,uint8_t index,uint8_t fragment[3])
{
    if(packet==NULL || fragment==NULL || generation>7U || index>=OCC_PACKET_BYTES)
        return OCC_INVALID;
    fragment[0]=(uint8_t)((generation<<5U)|index);
    fragment[1]=packet[index]; fragment[2]=occ_crc8(fragment,2U);
    return OCC_OK;
}
occ_result occ_cook_assembler_init(occ_cook_assembler *a,uint64_t timeout)
{
    occ_cook_assembler zero={0};
    if(a==NULL || timeout==0U || timeout>OCC_MAX_SEGMENT_NS) return OCC_INVALID;
    *a=zero; a->timeout_ns=timeout; return OCC_OK;
}
int occ_cook_fragment_accept(occ_cook_assembler *a,uint64_t now,
                             const uint8_t f[3],occ_packet *p)
{
    uint8_t index,generation;
    if(a==NULL || f==NULL || p==NULL || a->timeout_ns==0U) return OCC_INVALID;
    if(occ_crc8(f,2U)!=f[2]) { a->active=0U; return OCC_UNCERTAIN; }
    index=(uint8_t)(f[0]&31U); generation=(uint8_t)(f[0]>>5U);
    if(index>=OCC_PACKET_BYTES || (a->have_last && now<a->last_ns)) {
        a->active=0U; return OCC_UNCERTAIN;
    }
    if(a->have_last && now-a->last_ns>=a->timeout_ns) {
        a->active=0U; a->have_last=0U;
    }
    if(a->have_last && f[0]==a->last_tag && (a->active || index!=0U)) {
        if(f[1]!=a->last_data) { a->active=0U; return OCC_UNCERTAIN; }
        return OCC_OK; /* do not refresh last_ns on duplicate */
    }
    if(index==0U) { a->active=1U; a->generation=generation; a->next=0U; }
    if(!a->active || generation!=a->generation || index!=a->next) {
        a->active=0U; return OCC_UNCERTAIN;
    }
    a->bytes[index]=f[1]; a->next++;
    a->last_tag=f[0]; a->last_data=f[1]; a->last_ns=now; a->have_last=1U;
    if(a->next==OCC_PACKET_BYTES) {
        a->active=0U;
        return occ_packet_decode(a->bytes,p)==OCC_OK ? 1 : OCC_UNCERTAIN;
    }
    return OCC_OK;
}
static void reset(occ_cook_receiver *r)
{
    r->stage=0U; r->window=0U; r->count=0U; r->have_pair=0U;
    r->bytes[0]=r->bytes[1]=r->bytes[2]=0U;
}
occ_result occ_cook_receiver_init(occ_cook_receiver *r,uint64_t period)
{
    occ_cook_receiver zero={0};
    if(r==NULL || period==0U || period>OCC_MAX_SEGMENT_NS) return OCC_INVALID;
    *r=zero; r->period_ns=period; return OCC_OK;
}
int occ_cook_receiver_push(occ_cook_receiver *r,uint64_t now,int sample,
                            uint8_t f[3])
{
    uint8_t bit;
    unsigned i;
    if(r==NULL || f==NULL || r->period_ns==0U) return OCC_INVALID;
    if(r->have_last && (now<=r->last_ns || now-r->last_ns!=r->period_ns)) {
        r->last_ns=now; reset(r); return OCC_UNCERTAIN;
    }
    r->have_last=1U; r->last_ns=now;
    if(sample!=0 && sample!=1) { reset(r); return OCC_UNCERTAIN; }
    if(r->stage==0U) {
        r->window=(uint8_t)((((unsigned)r->window<<1U)|(unsigned)sample)&63U);
        if(r->count<6U) r->count++;
        if(r->count==6U && r->window==28U) { r->stage=1U; r->count=0U; }
        return OCC_OK;
    }
    if(!r->have_pair) { r->first=(uint8_t)sample; r->have_pair=1U; return OCC_OK; }
    r->have_pair=0U;
    if(occ_manchester_decode(r->first,(uint8_t)sample,&bit)!=OCC_OK) {
        reset(r); return OCC_UNCERTAIN;
    }
    if(r->count==0U) { r->ab=bit; r->count++; return OCC_OK; }
    if(r->count==OCC_COOK_FRAGMENT_BITS+1U) {
        if(bit!=r->ab) { reset(r); return OCC_UNCERTAIN; }
        for(i=0U;i<3U;++i) f[i]=r->bytes[i];
        reset(r); return 1;
    }
    r->bytes[(r->count-1U)/8U] |= (uint8_t)(bit<<((r->count-1U)%8U));
    r->count++; return OCC_OK;
}
