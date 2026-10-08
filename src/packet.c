/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/packet.h"
static void put16(uint8_t *b, uint16_t v)
{ b[0]=(uint8_t)v; b[1]=(uint8_t)(v>>8U); }
static void put32(uint8_t *b, uint32_t v)
{
    unsigned i;
    for(i=0U;i<4U;++i) b[i]=(uint8_t)(v>>(i*8U));
}
static uint16_t get16(const uint8_t *b)
{ return (uint16_t)((uint16_t)b[0] | (uint16_t)((uint16_t)b[1]<<8U)); }
static uint32_t get32(const uint8_t *b)
{
    unsigned i; uint32_t v=0U;
    for(i=0U;i<4U;++i) v |= (uint32_t)b[i]<<(i*8U);
    return v;
}
static int valid_kind(uint8_t k) { return k>=1U && k<=5U; }
occ_result occ_packet_encode(const occ_packet *p, uint8_t out[OCC_PACKET_BYTES])
{
    if(p==NULL || out==NULL || !valid_kind(p->kind)) return OCC_INVALID;
    out[0]=0x4fU; out[1]=0x43U; out[2]=1U; out[3]=p->kind;
    put32(out+4,p->board); put32(out+8,p->session);
    put16(out+12,p->sequence); put16(out+14,p->status); put32(out+16,p->build);
    put16(out+20,occ_crc16(out,20U));
    return OCC_OK;
}
occ_result occ_packet_decode(const uint8_t in[OCC_PACKET_BYTES], occ_packet *p)
{
    occ_packet decoded;
    if(in==NULL || p==NULL) return OCC_INVALID;
    if(in[0]!=0x4fU || in[1]!=0x43U || in[2]!=1U || !valid_kind(in[3]) ||
       get16(in+20)!=occ_crc16(in,20U)) return OCC_UNCERTAIN;
    decoded.kind=in[3]; decoded.board=get32(in+4); decoded.session=get32(in+8);
    decoded.sequence=get16(in+12); decoded.status=get16(in+14);
    decoded.build=get32(in+16);
    *p=decoded; return OCC_OK;
}
occ_result occ_bytes_to_bits(const uint8_t *bytes, size_t n,
                            uint8_t *bits, size_t capacity)
{
    size_t i;
    if(bytes==NULL || bits==NULL || n>SIZE_MAX/8U) return OCC_INVALID;
    if(n*8U>capacity) return OCC_CAPACITY;
    for(i=0U;i<n*8U;++i) bits[i]=(uint8_t)(((unsigned)bytes[i/8U]>>(i%8U))&1U);
    return OCC_OK;
}
occ_result occ_bits_to_bytes(const uint8_t *bits, size_t n,
                            uint8_t *bytes, size_t capacity)
{
    size_t i;
    if(bits==NULL || bytes==NULL || n%8U!=0U) return OCC_INVALID;
    if(n/8U>capacity) return OCC_CAPACITY;
    for(i=0U;i<n;++i) if(bits[i]>1U) return OCC_INVALID;
    for(i=0U;i<n/8U;++i) bytes[i]=0U;
    for(i=0U;i<n;++i) bytes[i/8U] |= (uint8_t)(bits[i]<<(i%8U));
    return OCC_OK;
}
void occ_tracker_init(occ_tracker *t, uint32_t board)
{
    occ_tracker zero={0};
    if(t==NULL) return;
    *t=zero; t->expected_board=board;
}
occ_result occ_tracker_accept(occ_tracker *t, uint64_t now,
                             const occ_packet *p, int *progresses)
{
    uint16_t distance;
    int progress;
    occ_result r;
    if(t==NULL || p==NULL || progresses==NULL || !valid_kind(p->kind))
        return OCC_INVALID;
    if(p->board!=t->expected_board ||
       (t->have_session && p->session!=t->session)) return OCC_UNCERTAIN;
    distance=(uint16_t)(p->sequence-t->sequence);
    if(t->have_session && distance>=UINT16_C(0x8000)) return OCC_UNCERTAIN;
    if(t->have_session && distance==0U &&
       (p->kind!=t->kind || p->status!=t->status || p->build!=t->build))
        return OCC_UNCERTAIN;
    progress=!t->have_session || distance!=0U;
    r=occ_observe(&t->observation,now,1,1,progress);
    if(r!=OCC_OK) return r;
    if(progress) {
        t->session=p->session; t->sequence=p->sequence; t->have_session=1U;
        t->kind=p->kind; t->status=p->status; t->build=p->build;
    }
    *progresses=progress; return OCC_OK;
}
