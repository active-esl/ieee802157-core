/* SPDX-License-Identifier: GPL-3.0-only */
/* OS adapter sample: no physical LED/camera, no driver bindings. */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "occ/player.h"
#include "occ/packet.h"
#include "occ/receiver.h"
#include "occ/colour.h"
static occ_segment segments[OCC_PACKET_BITS+4U];
static uint8_t bytes[OCC_PACKET_BYTES],bits[OCC_PACKET_BITS];
static occ_wave wave={segments,0U,OCC_PACKET_BITS+4U};
int main(void)
{
    occ_packet source={2U,42U,99U,1U,0U,1234U},decoded;
    occ_ufsook_profile profile={30U,1U,4U,1200U,1U};
    occ_led_capabilities simulated={1U,1U,0U};
    occ_player player;
    occ_receiver receiver;
    occ_packet received={0};
    uint64_t stamp=0U;
    unsigned i;
    int rc=OCC_OK;
    uint64_t start=(uint64_t)k_uptime_get()*UINT64_C(1000000),next;
    uint8_t channels;
    if(occ_packet_encode(&source,bytes)!=OCC_OK ||
       occ_packet_decode(bytes,&decoded)!=OCC_OK || decoded.board!=42U ||
       occ_bytes_to_bits(bytes,sizeof(bytes),bits,sizeof(bits))!=OCC_OK ||
       occ_ufsook_encode(&profile,bits,sizeof(bits),&wave)!=OCC_OK ||
       occ_player_init(&player,&wave,&simulated,start,0U)!=OCC_OK ||
       occ_player_step(&player,start,&channels,&next)!=OCC_OK) {
        printk("OCC_ZEPHYR_FAIL\n"); return 1;
    }
    if(occ_receiver_init(&receiver,OCC_RX_UFSOOK,33333333U,500000U)!=OCC_OK)
        return 1;
    /* Simulated recovered-symbol input verifies decoder linkage/runtime;
     * these values are NOT camera measurements. */
    {
        const int delimiter[4]={2,2,0,1};
        for(i=0U;i<4U;++i) {
            occ_receiver_push(&receiver,stamp,delimiter[i],&received);
            stamp+=33333333U;
        }
        for(i=0U;i<OCC_PACKET_BITS;++i) {
            occ_receiver_push(&receiver,stamp,0,&received); stamp+=33333333U;
            occ_receiver_push(&receiver,stamp,bits[i],&received); stamp+=33333333U;
        }
        for(i=0U;i<4U;++i) {
            rc=occ_receiver_push(&receiver,stamp,delimiter[i],&received);
            stamp+=33333333U;
        }
    }
    if(rc!=OCC_PACKET_READY || received.board!=source.board) {
        printk("OCC_ZEPHYR_DECODE_FAIL\n"); return 1;
    }
    {
        const occ_colour_profile colour={125000000U,{1U,2U,4U,8U}};
        occ_colour_receiver colour_rx;
        stamp=0U;
        if(occ_colour_encode(&colour,bytes,&wave)!=OCC_OK ||
           occ_colour_receiver_init(&colour_rx,colour.symbol_ns,10000000U)!=OCC_OK)
            return 1;
        for(i=0U;i<wave.count;++i) {
            unsigned c;
            int symbol=OCC_COLOUR_DARK;
            for(c=0U;c<4U;++c)
                if(wave.segments[i].active_mask==colour.channels[c]) symbol=(int)c;
            rc=occ_colour_receiver_push(&colour_rx,stamp,symbol,&received);
            stamp+=colour.symbol_ns;
        }
        if(rc!=OCC_PACKET_READY || received.board!=source.board ||
           received.session!=source.session || received.sequence!=source.sequence) {
            printk("OCC_ZEPHYR_COLOUR_FAIL\n"); return 1;
        }
        printk("OCC_ZEPHYR_COLOUR_PASS symbols=%u virtual_ns=%llu\n",
               (unsigned)wave.count,(unsigned long long)stamp);
    }
    /* The virtual capabilities above are NOT a claim about Zephyr's tick timer.
     * k_uptime_get provides only the sample's epoch, not carrier-edge scheduling.
     * A real adapter must provide independently measured high-resolution timing. */
    printk("OCC_ZEPHYR_PASS board=%u segments=%u first_channels=%u\n",
           decoded.board,(unsigned)wave.count,(unsigned)channels);
    return 0;
}
