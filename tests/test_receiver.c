/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/receiver.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const occ_packet source={4U,0x12345678U,0x10203040U,65535U,0x4321U,0x87654321U};
static unsigned count;
static unsigned sample_index;
static unsigned fault_index;
static int fault_mode;
static uint64_t timestamp;
static int push(occ_receiver *r,int sample,occ_packet *out)
{
    int rc;
    if(fault_mode && sample_index++==fault_index) {
        if(fault_mode==1) { timestamp+=r->period_ns; return OCC_OK; }
        sample=OCC_SAMPLE_ERASURE;
    }
    rc=occ_receiver_push(r,timestamp,sample,out);
    timestamp+=r->period_ns;
    if(rc==OCC_PACKET_READY) ++count;
    return rc;
}
static void feed(occ_receiver *r,const uint8_t *bits,occ_packet *out)
{
    static const int preamble[]={0,1,1,1,0,0};
    unsigned i;
    if(r->mode==OCC_RX_UFSOOK) {
        push(r,2,out); push(r,2,out); push(r,0,out); push(r,1,out);
        for(i=0U;i<OCC_PACKET_BITS;++i) {
            push(r,1,out); push(r,bits[i]==0U?1:0,out);
        }
        push(r,2,out); push(r,2,out); push(r,1,out); push(r,0,out);
    } else if(r->mode==OCC_RX_S2PSK) {
        for(i=0U;i<4U;++i) push(r,1,out);
        for(i=0U;i<OCC_PACKET_BITS;++i) { push(r,0,out); push(r,bits[i],out); }
        for(i=0U;i<4U;++i) push(r,1,out);
    } else {
        for(i=0U;i<6U;++i) push(r,preamble[i],out);
        push(r,1,out); push(r,0,out); /* start Ab=1 */
        for(i=0U;i<OCC_PACKET_BITS;++i) {
            push(r,bits[i],out); push(r,bits[i]==0U?1:0,out);
        }
        push(r,1,out); push(r,0,out); /* end Ab=1 */
    }
}
static void packet_tests(void)
{
    uint8_t bytes[OCC_PACKET_BYTES], altered[OCC_PACKET_BYTES],bits[OCC_PACKET_BITS];
    occ_packet out;
    unsigned i,j;
    assert(occ_packet_encode(&source,bytes)==OCC_OK);
    assert(bytes[0]==0x4fU && bytes[2]==1U && bytes[4]==0x78U);
    assert(occ_packet_decode(bytes,&out)==OCC_OK && out.build==source.build);
    for(i=0U;i<OCC_PACKET_BYTES;++i) for(j=0U;j<8U;++j) {
        memcpy(altered,bytes,sizeof(bytes)); altered[i]^=(uint8_t)(1U<<j);
        assert(occ_packet_decode(altered,&out)==OCC_UNCERTAIN);
    }
    assert(occ_bytes_to_bits(bytes,sizeof(bytes),bits,sizeof(bits))==OCC_OK);
    assert(occ_bits_to_bytes(bits,sizeof(bits),altered,sizeof(altered))==OCC_OK);
    assert(memcmp(bytes,altered,sizeof(bytes))==0);
    bits[0]=2U;
    assert(occ_bits_to_bytes(bits,sizeof(bits),altered,sizeof(altered))==OCC_INVALID);
}
static void receiver_tests(void)
{
    uint8_t bytes[OCC_PACKET_BYTES],bits[OCC_PACKET_BITS];
    occ_packet out={0};
    occ_receiver r;
    int mode;
    occ_packet_encode(&source,bytes); occ_bytes_to_bits(bytes,sizeof(bytes),bits,sizeof(bits));
    for(mode=OCC_RX_UFSOOK;mode<=OCC_RX_COOK;++mode) {
        assert(occ_receiver_init(&r,(occ_rx_mode)mode,1000U,10U)==OCC_OK);
        timestamp=0U; count=0U; feed(&r,bits,&out);
        assert(count==1U && out.board==source.board && out.sequence==65535U);
        /* Corrupted encoded packet produces no accepted observation. */
        bits[80]^=1U; count=0U; feed(&r,bits,&out); assert(count==0U); bits[80]^=1U;
        count=0U; feed(&r,bits,&out); assert(count==1U); /* reacquire */
        assert(push(&r,OCC_SAMPLE_ERASURE,&out)==OCC_UNCERTAIN);
        timestamp+=1000U; assert(push(&r,0,&out)==OCC_UNCERTAIN);
        count=0U; feed(&r,bits,&out); assert(count==1U);
        assert(occ_receiver_push(&r,timestamp-2000U,0,&out)==OCC_UNCERTAIN);
    }
}
static void tracker_tests(void)
{
    occ_tracker t;
    occ_packet p=source;
    int progresses=0;
    occ_tracker_init(&t,p.board);
    assert(occ_tracker_accept(&t,100U,&p,&progresses)==OCC_OK && progresses);
    assert(occ_tracker_accept(&t,101U,&p,&progresses)==OCC_OK && !progresses);
    p.status^=1U;
    assert(occ_tracker_accept(&t,102U,&p,&progresses)==OCC_UNCERTAIN);
    p=source; p.build^=1U;
    assert(occ_tracker_accept(&t,102U,&p,&progresses)==OCC_UNCERTAIN);
    p=source; p.kind=3U;
    assert(occ_tracker_accept(&t,102U,&p,&progresses)==OCC_UNCERTAIN);
    assert(t.observation.last_signal_ns==101U &&
           t.observation.last_progress_ns==100U);
    p=source;
    p.sequence=0U;
    assert(occ_tracker_accept(&t,102U,&p,&progresses)==OCC_OK && progresses);
    p.sequence=65535U;
    assert(occ_tracker_accept(&t,103U,&p,&progresses)==OCC_UNCERTAIN);
    p.sequence=1U; p.session++;
    assert(occ_tracker_accept(&t,104U,&p,&progresses)==OCC_UNCERTAIN);
    p=source; p.board++;
    assert(occ_tracker_accept(&t,104U,&p,&progresses)==OCC_UNCERTAIN);
}
static void fault_positions(void)
{
    uint8_t bytes[OCC_PACKET_BYTES],bits[OCC_PACKET_BITS];
    occ_packet out={0};
    occ_receiver r;
    unsigned mode,position,samples,j;
    int fault;
    occ_packet_encode(&source,bytes);
    occ_bytes_to_bits(bytes,sizeof(bytes),bits,sizeof(bits));
    for(mode=OCC_RX_UFSOOK;mode<=OCC_RX_COOK;++mode) {
        samples=mode==OCC_RX_COOK ? 6U+2U*(OCC_PACKET_BITS+2U) : 8U+2U*OCC_PACKET_BITS;
        for(fault=1;fault<=2;++fault) for(position=0U;position<samples;++position) {
            assert(occ_receiver_init(&r,(occ_rx_mode)mode,1000U,10U)==OCC_OK);
            count=0U; timestamp=0U; sample_index=0U;
            fault_index=position; fault_mode=fault;
            feed(&r,bits,&out);
            assert(count==0U); /* one missing/erased sample anywhere rejects */
        }
        fault_mode=0;
        for(j=0U;j<2U;++j) {
            assert(occ_receiver_init(&r,(occ_rx_mode)mode,1000U,10U)==OCC_OK);
            count=0U; timestamp=0U;
            for(position=0U;position<1000U;++position) push(&r,(int)j,&out);
            assert(count==0U); /* constant dark/bright isn't a valid packet */
        }
    }
}
int main(void)
{
    packet_tests(); receiver_tests(); tracker_tests(); fault_positions();
    puts("PASS: 176 single-bit corruptions rejected; every symbol drop/erasure rejected in 3 receivers; constant light rejected; reacquisition; sequence/source/session gates");
    return 0;
}
