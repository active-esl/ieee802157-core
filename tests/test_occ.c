/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/occ.h"
#ifdef NDEBUG
#undef NDEBUG /* Fixture assertions must run in release builds too. */
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void primitives(void)
{
    uint8_t bit=9U;
    occ_segment s={10000000U,1000U,1U,3U,2U};
    assert(occ_segment_level(&s,0U,&bit)==OCC_OK && bit==1U);
    assert(occ_segment_level(&s,500000U,&bit)==OCC_OK && bit==2U);
    assert(occ_segment_level(&s,1000000U,&bit)==OCC_OK && bit==1U);
    assert(occ_segment_level(&s,10000000U,&bit)==OCC_INVALID);
    assert(occ_ufsook_demod_pair(0,0,&bit)==OCC_OK && bit==0U);
    assert(occ_ufsook_demod_pair(0,1,&bit)==OCC_OK && bit==1U);
    assert(occ_ufsook_demod_pair(-1,0,&bit)==OCC_UNCERTAIN);
    assert(occ_s2psk_demod_pair(1,0,&bit)==OCC_OK && bit==0U);
    assert(occ_s2psk_demod_pair(1,1,&bit)==OCC_OK && bit==1U);
    assert(occ_s2psk_demod_pair(0,0,&bit)==OCC_OK && bit==1U);
    assert(occ_s2psk_line_decode(0U,1U,&bit)==OCC_OK && bit==1U);
    assert(occ_s2psk_line_decode(1U,0U,&bit)==OCC_UNCERTAIN);
    assert(occ_manchester_decode(0U,1U,&bit)==OCC_OK && bit==0U);
    assert(occ_manchester_decode(1U,0U,&bit)==OCC_OK && bit==1U);
    assert(occ_manchester_decode(1U,1U,&bit)==OCC_UNCERTAIN);
    assert(occ_crc16((const uint8_t *)"123456789",9U)==0x29b1U);
}
static void encoders(void)
{
    occ_segment storage[256], before;
    occ_wave w={storage,0U,256U};
    const uint8_t bits[]={0U,1U};
    uint8_t a,b,bit;
    uint64_t total;
    occ_ufsook_profile u={30U,1U,4U,1200U,1U};
    occ_s2psk_profile s={10U,1000U,1U,2U};
    occ_cook_profile c={2200U,2U,1U};
    size_t i;
    assert(occ_ufsook_encode(&u,bits,2U,&w)==OCC_OK && w.count==6U);
    assert(w.segments[0].rate_num==1200U);
    assert(w.segments[2].rate_num==240U && w.segments[2].rate_den==2U);
    assert(w.segments[3].rate_num==210U && w.segments[3].rate_den==2U);
    for (i=2U;i<4U;++i) {
        assert(occ_segment_level(&w.segments[i],2000000U,&a)==OCC_OK);
        assert(occ_segment_level(&w.segments[i],35333333U,&b)==OCC_OK);
        assert(occ_ufsook_demod_pair(a,b,&bit)==OCC_OK && bit==bits[i-2U]);
    }
    assert(occ_wave_duration(&w,&total)==OCC_OK && total==399999996U);
    assert(occ_wave_level(&w,total,&bit)==OCC_OK && bit==0U);
    before=w.segments[0]; w.capacity=1U;
    assert(occ_ufsook_encode(&u,bits,2U,&w)==OCC_CAPACITY);
    assert(memcmp(&before,&storage[0],sizeof(before))==0);
    w.capacity=256U;
    u.channel=3U; assert(occ_ufsook_encode(&u,bits,2U,&w)==OCC_INVALID);
    assert(occ_s2psk_encode(&s,bits,2U,&w)==OCC_OK && w.count==12U);
    assert(w.segments[0].inverted_mask==0U);
    assert(w.segments[4].inverted_mask==2U && w.segments[5].inverted_mask==2U);
    assert(w.segments[6].inverted_mask==2U && w.segments[7].inverted_mask==0U);
    s.channel_b=1U; assert(occ_s2psk_encode(&s,bits,2U,&w)==OCC_INVALID);
    assert(occ_cook_encode(&c,1U,bits,2U,&w)==OCC_OK && w.count==28U);
    for(i=0U;i<14U;++i) {
        assert(w.segments[i].active_mask==w.segments[i+14U].active_mask);
        assert(w.segments[i].duration_ns==454545U);
    }
    assert(w.segments[0].active_mask==0U);
    assert(w.segments[1].active_mask==1U);
    assert(w.segments[6].active_mask==1U && w.segments[7].active_mask==0U);
    assert(w.segments[8].active_mask==0U && w.segments[9].active_mask==1U);
    assert(w.segments[10].active_mask==1U && w.segments[11].active_mask==0U);
    assert(occ_cook_encode(&c,2U,bits,2U,&w)==OCC_INVALID);
}
static void observations(void)
{
    occ_observation o={0};
    assert(occ_observation_status(&o,0U,10U,20U)==OCC_NO_SIGNAL);
    assert(occ_observe(&o,1U,1,0,0)==OCC_OK);
    assert(occ_observation_status(&o,1U,10U,20U)==OCC_UNCERTAIN_SIGNAL);
    assert(occ_observe(&o,2U,1,1,1)==OCC_OK);
    assert(occ_observation_status(&o,3U,10U,20U)==OCC_VALID_OBSERVATION);
    assert(occ_observe(&o,21U,1,1,0)==OCC_OK); /* repeated frame, no progress */
    assert(occ_observation_status(&o,22U,10U,20U)==OCC_STALE_OBSERVATION);
    assert(occ_observation_status(&o,31U,10U,20U)==OCC_NO_SIGNAL);
    assert(occ_observe(&o,1U,1,1,1)==OCC_INVALID);
}
int main(void)
{
    primitives(); encoders(); observations();
    puts("PASS: waveform profiles, demapper primitives, CRC vector, observation timeouts");
    return 0;
}
