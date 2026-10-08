/* SPDX-License-Identifier: GPL-3.0-only */
/* Finite offline two-ROI adapter with calibrated symbol epoch/clock.
 * No camera, LED, USB, serial, network or hardware timer API. */
#include "occ/spatial.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
static int receive(occ_receiver *r,occ_tracker *t,uint64_t stamp,int sample,
                   unsigned *packets)
{
    occ_packet p; int progress;
    int rc=occ_receiver_push(r,stamp,sample,&p);
    if(rc==OCC_PACKET_READY &&
       occ_tracker_accept(t,stamp,&p,&progress)==OCC_OK) (*packets)++;
    return rc;
}
int main(int argc,char **argv)
{
    /* Experiment scene format is explicitly fixed; source geometry is not
     * inferred or interchangeable with an RGB package. */
    uint8_t frame[64U*64U];
    occ_gray_image im={frame,64U,64U,64U};
    occ_roi a={12U,28U,8U,8U},b={44U,28U,8U,8U};
    const occ_thresholds p={70U,180U,100U,150U,2U,253U,250U};
    occ_symbol_group g;
    occ_receiver r;
    occ_tracker tracker;
    uint64_t time,last=0U,stamp;
    unsigned frames=0U,packets=0U,uncertain=0U;
    int scan,sample,symbol,rc;
    FILE *timestamps;
    if(argc!=2) { fputs("usage: occ_decode_spatial TIMESTAMPS (64x64 gray on stdin)\n",stderr); return 2; }
    timestamps=fopen(argv[1],"r"); if(timestamps==NULL) return 2;
    if(occ_symbol_group_init(&g,0U,100000000U,70000000U,2U)!=OCC_OK ||
       occ_receiver_init(&r,OCC_RX_S2PSK,100000000U,1000U)!=OCC_OK) {
        fclose(timestamps); return 2;
    }
    occ_tracker_init(&tracker,0x12345678U);
    while((scan=fscanf(timestamps,"%" SCNu64,&time))==1) {
        if(++frames>10000U || fread(frame,1U,sizeof(frame),stdin)!=sizeof(frame) ||
           occ_s2psk_image_sample(&im,&a,&b,&p,&sample)!=OCC_OK) {
            fclose(timestamps); return 2;
        }
        last=time; rc=occ_symbol_group_push(&g,time,sample,&stamp,&symbol);
        if(rc==OCC_SYMBOL_READY) {
            if(receive(&r,&tracker,stamp,symbol,&packets)<0) uncertain++;
        } else if(rc<0) uncertain++;
    }
    if(scan!=EOF || fgetc(stdin)!=EOF || frames==0U || UINT64_MAX-last<33333333U) {
        fclose(timestamps); return 2;
    }
    rc=occ_symbol_group_flush(&g,last+33333333U,&stamp,&symbol);
    if(rc==OCC_SYMBOL_READY) {
        if(receive(&r,&tracker,stamp,symbol,&packets)<0) uncertain++;
    } else uncertain++;
    printf("{\"frames\":%u,\"packets\":%u,\"uncertain\":%u}\n",frames,packets,uncertain);
    fclose(timestamps); return 0;
}
