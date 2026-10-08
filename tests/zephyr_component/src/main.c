/* SPDX-License-Identifier: GPL-3.0-only */
#include <zephyr/sys/printk.h>
#ifdef CONFIG_LIGHT_COMMS
#include <occ/colour.h>
#include <occ/player.h>
static occ_segment segments[OCC_COLOUR_SYMBOLS];
int main(void)
{
    const occ_packet source={2U,42U,99U,1U,0U,1234U};
    const occ_colour_profile profile={125000000U,{1U,2U,4U,8U}};
    const occ_led_capabilities caps={15U,1U,0U};
    occ_wave wave={segments,0U,OCC_COLOUR_SYMBOLS};
    occ_player player;
    uint8_t bytes[OCC_PACKET_BYTES],channels;
    uint64_t next;
    if(occ_packet_encode(&source,bytes)!=OCC_OK ||
       occ_colour_encode(&profile,bytes,&wave)!=OCC_OK ||
       occ_player_init(&player,&wave,&caps,0U,0U)!=OCC_OK ||
       occ_player_step(&player,0U,&channels,&next)!=OCC_OK)
        return 1;
    printk("LIGHT_COMMS_SUBSET_PASS\n");
    return 0;
}
#else
int main(void)
{
    printk("LIGHT_COMMS_DISABLED_PASS\n");
    return 0;
}
#endif
