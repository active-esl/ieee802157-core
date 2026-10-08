/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/colour.h"
#include "occ/player.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const occ_colour_profile profile={125000000U,{1,2,4,8}};
static const occ_colour_calibration cal={{5,5,5},
    {{100,5,3},{4,100,7},{5,10,100},{80,85,75}},40,250,400,200};
static uint32_t random_state=0x43b217e9U;
static uint32_t random32(void)
{
    random_state^=random_state<<13U; random_state^=random_state>>17U;
    random_state^=random_state<<5U; return random_state;
}
static int symbol(uint8_t mask)
{
    unsigned i;
    for(i=0;i<4U;i++) if(mask==profile.channels[i]) return (int)i;
    return OCC_COLOUR_DARK;
}
static unsigned feed(occ_colour_receiver *r,const occ_wave *w,uint64_t *t,
    int dropped,int changed,occ_packet *out)
{
    size_t i; unsigned ready=0;
    for(i=0;i<w->count;i++) {
        int s=symbol(w->segments[i].active_mask);
        if((int)i==changed) s=(s+1)%4;
        if((int)i!=dropped && occ_colour_receiver_push(r,*t,s,out)==OCC_PACKET_READY)
            ready++;
        *t+=profile.symbol_ns;
    }
    return ready;
}
int main(void)
{
    occ_segment segments[OCC_COLOUR_SYMBOLS];
    occ_wave w={segments,0,OCC_COLOUR_SYMBOLS};
    occ_packet p={2,42,7,0,0,1234},out;
    uint8_t bytes[OCC_PACKET_BYTES]; uint64_t total,t; unsigned i,k;
    occ_colour_receiver r; occ_tracker tracker; int progress;
    assert(occ_packet_encode(&p,bytes)==OCC_OK);
    assert(occ_colour_encode(&profile,bytes,&w)==OCC_OK);
    assert(occ_wave_duration(&w,&total)==OCC_OK && total==13750000000ULL);
    assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
    occ_tracker_init(&tracker,42); t=0;
    for(i=0;i<8U;i++) {
        p.sequence=(uint16_t)i;
        assert(occ_packet_encode(&p,bytes)==OCC_OK);
        assert(occ_colour_encode(&profile,bytes,&w)==OCC_OK);
        assert(feed(&r,&w,&t,-1,-1,&out)==1U);
        assert(out.sequence==i && out.board==42);
        assert(occ_tracker_accept(&tracker,t,&out,&progress)==OCC_OK && progress);
    }
    assert(t==110000000000ULL);
    assert(occ_tracker_accept(&tracker,t+1U,&out,&progress)==OCC_OK && !progress);
    assert(occ_observation_status(&tracker.observation,t+1U,41250000000ULL,
        68750000000ULL)==OCC_VALID_OBSERVATION);
    assert(occ_tracker_accept(&tracker,t+70000000000ULL,&out,&progress)==OCC_OK && !progress);
    assert(occ_observation_status(&tracker.observation,t+70000000000ULL,
        41250000000ULL,68750000000ULL)==OCC_STALE_OBSERVATION);
    assert(occ_observation_status(&tracker.observation,t+120000000000ULL,
        41250000000ULL,68750000000ULL)==OCC_NO_SIGNAL);
    out.board=43;
    assert(occ_tracker_accept(&tracker,t+70000000001ULL,&out,&progress)==OCC_UNCERTAIN);
    for(i=OCC_COLOUR_SYNC;i<OCC_COLOUR_SYNC+OCC_COLOUR_DATA;i++) {
        assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
        t=0;
        assert(feed(&r,&w,&t,-1,(int)i,&out)==0U);
        assert(feed(&r,&w,&t,-1,-1,&out)==1U);
    }
    for(i=0;i<OCC_COLOUR_SYMBOLS;i++) {
        assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
        t=0;
        assert(feed(&r,&w,&t,(int)i,-1,&out)==0U);
        /* At most two whole repeated frames to resynchronize, including loss
         * of the final guard symbol and rejection of the next timestamp. */
        k=feed(&r,&w,&t,-1,-1,&out);
        k+=feed(&r,&w,&t,-1,-1,&out);
        assert(k>=1U);
    }
    /* Sync, trailer and both guard symbols must reject a changed frame too. */
    for(i=0;i<OCC_COLOUR_SYMBOLS;i++) {
        assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
        t=0;
        assert(feed(&r,&w,&t,-1,(int)i,&out)==0U);
        k=feed(&r,&w,&t,-1,-1,&out);
        k+=feed(&r,&w,&t,-1,-1,&out);
        assert(k>=1U);
    }
    {
        occ_colour_profile bad=profile;
        occ_wave short_wave={segments,17U,OCC_COLOUR_SYMBOLS-1U};
        occ_segment before=segments[0];
        assert(occ_colour_encode(&profile,bytes,&short_wave)==OCC_CAPACITY);
        assert(short_wave.count==17U && !memcmp(&before,&segments[0],sizeof(before)));
        bad.channels[1]=bad.channels[0];
        assert(occ_colour_encode(&bad,bytes,&w)==OCC_INVALID);
        bad=profile; bad.channels[0]=3U;
        assert(occ_colour_encode(&bad,bytes,&w)==OCC_INVALID);
        assert(occ_colour_receiver_init(&r,0,0)==OCC_INVALID);
        assert(occ_colour_receiver_init(&r,profile.symbol_ns,profile.symbol_ns/2U)==OCC_INVALID);
        assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
        assert(occ_colour_receiver_push(&r,UINT64_MAX-profile.symbol_ns,0,&out)==OCC_OK);
        assert(occ_colour_receiver_push(&r,UINT64_MAX,1,&out)==OCC_OK);
        assert(occ_colour_receiver_push(&r,0,0,&out)==OCC_UNCERTAIN);
        assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
        assert(occ_colour_receiver_push(&r,0,0,&out)==OCC_OK);
        assert(occ_colour_receiver_push(&r,0,1,&out)==OCC_UNCERTAIN);
        assert(occ_colour_receiver_push(&r,profile.symbol_ns,-1,&out)==OCC_UNCERTAIN);
        assert(occ_colour_receiver_push(&r,2U*profile.symbol_ns,5,&out)==OCC_UNCERTAIN);
    }
    assert(occ_colour_calibration_check(&cal)==OCC_OK);
    {
        occ_player player;
        occ_led_capabilities caps={15U,125000000U,10000000U};
        uint64_t deadline; uint8_t channels;
        assert(occ_player_init(&player,&w,&caps,0,10000000U)==OCC_OK);
        t=0;
        for(i=0;i<w.count;++i) {
            assert(occ_player_step(&player,t,&channels,&deadline)==OCC_OK);
            assert(channels==w.segments[i].active_mask && deadline==t+profile.symbol_ns);
            t=deadline;
        }
        assert(occ_player_step(&player,t,&channels,&deadline)==OCC_OK && channels==0U);
        assert(occ_player_init(&player,&w,&caps,0,10000000U)==OCC_OK);
        assert(occ_player_step(&player,10000001U,&channels,&deadline)==OCC_UNCERTAIN && channels==0U);
        assert(occ_player_step(&player,profile.symbol_ns,&channels,&deadline)==OCC_UNCERTAIN && channels==0U);
        caps.logical_channels=7U;
        assert(occ_player_init(&player,&w,&caps,0,10000000U)==OCC_UNSUPPORTED);
    }
    for(i=0;i<4U;i++) {
        uint16_t rgb[3]; int s=-1;
        for(k=0;k<3U;k++) rgb[k]=(uint16_t)(cal.prototypes[i][k]+cal.dark[k]);
        assert(occ_colour_classify(&cal,rgb,&s)==OCC_OK && s==(int)i);
    }
    {
        uint16_t saturated[3]={255,10,10},ambiguous[3]={65,65,5}; int s=-1;
        occ_colour_calibration bad=cal;
        assert(occ_colour_classify(&cal,saturated,&s)==OCC_UNCERTAIN && s==-1);
        assert(occ_colour_classify(&cal,ambiguous,&s)==OCC_UNCERTAIN && s==-1);
        memcpy(bad.prototypes[1],bad.prototypes[0],sizeof(bad.prototypes[0]));
        assert(occ_colour_calibration_check(&bad)==OCC_UNCERTAIN);
    }
    /* Finite deterministic malformed inputs exercise memory/UB and rejection
     * handling, not an authentication or statistical false-positive proof. */
    assert(occ_colour_receiver_init(&r,profile.symbol_ns,10000000U)==OCC_OK);
    t=0;
    for(i=0;i<100000U;++i) {
        uint16_t rgb[3]; int classified=-1; occ_result rc;
        t+=(uint64_t)(random32()%250000000U);
        (void)occ_colour_receiver_push(&r,t,(int)(random32()%8U)-2,&out);
        for(k=0;k<3U;++k) rgb[k]=(uint16_t)(random32()%512U);
        rc=occ_colour_classify(&cal,rgb,&classified);
        assert(rc==OCC_OK || rc==OCC_UNCERTAIN);
        if(rc==OCC_OK) assert(classified>=0 && classified<=OCC_COLOUR_DARK);
        else assert(classified==-1);
    }
    puts("colour: eight frames/110s, all 110 corruptions/gaps, player abort, 100000 malformed inputs PASS");
    return 0;
}
