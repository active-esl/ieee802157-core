/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/player.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
int main(void)
{
    occ_segment segs[16];
    occ_wave w={segs,0U,16U};
    occ_ufsook_profile u={30U,1U,4U,1200U,1U};
    occ_s2psk_profile s={10U,1000U,1U,2U};
    const uint8_t bits[]={0U,1U};
    occ_led_capabilities c={3U,1U,0U};
    occ_player p;
    uint8_t channels,expected;
    uint64_t now,next,total;
    unsigned edges=0U;
    assert(occ_ufsook_encode(&u,bits,2U,&w)==OCC_OK);
    assert(occ_player_init(&p,&w,&c,100U,0U)==OCC_OK);
    assert(occ_player_step(&p,0U,&channels,&next)==OCC_OK && channels==0U && next==100U);
    occ_wave_duration(&w,&total);
    now=100U;
    do {
        assert(occ_player_step(&p,now,&channels,&next)==OCC_OK);
        assert(occ_wave_level(&w,now-100U,&expected)==OCC_OK);
        assert(channels==expected);
        if(next!=UINT64_MAX) assert(next>now);
        now=next; edges++;
    } while(!p.finished && edges<10000U);
    assert(p.finished && edges<10000U);
    assert(occ_player_init(&p,&w,&c,100U,0U)==OCC_OK);
    assert(occ_player_step(&p,101U,&channels,&next)==OCC_UNCERTAIN && channels==0U);
    c.min_interval_ns=10000000U;
    assert(occ_player_init(&p,&w,&c,0U,0U)==OCC_UNSUPPORTED);
    c.min_interval_ns=1U; c.jitter_ns=100U;
    assert(occ_player_init(&p,&w,&c,0U,0U)==OCC_UNSUPPORTED);
    assert(occ_s2psk_encode(&s,bits,2U,&w)==OCC_OK);
    c.logical_channels=1U; c.jitter_ns=0U;
    assert(occ_player_init(&p,&w,&c,0U,0U)==OCC_UNSUPPORTED);
    puts("PASS: all waveform deadlines, lateness abort/all-off, rate/jitter/channel capability rejection");
    return 0;
}
