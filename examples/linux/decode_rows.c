/* SPDX-License-Identifier: GPL-3.0-only */
/* Calibrated rolling-shutter experiment, fixed64x720 scene profile.
 * Optical clock4400Hz rounded to227272ns; rowread40us, exposure10us.
 * Camera phase80us relative to known TX epoch each frame. This is explicit
 * test calibration, NOT blind acquisition or evidence for a physical webcam. */
#include "occ/rolling.h"
#include <inttypes.h>
#include <stdio.h>
int main(int argc,char **argv)
{
    static uint8_t frame[64U*720U];
    occ_gray_image image={frame,64U,720U,64U};
    const occ_thresholds thresholds={70U,180U,100U,150U,2U,253U,250U};
    occ_roi roi={28U,0U,8U,720U};
    const occ_rolling_profile profile={227272U,40000U,10000U,3U};
    occ_cook_assembler assembler;
    occ_tracker tracker;
    occ_packet packet;
    unsigned frames=0U,packets=0U,uncertain=0U,image_uncertain;
    uint64_t capture,last=0U;
    int scan,rc,progress;
    FILE *times;
    if(argc!=2) { fputs("usage: occ_decode_rows TIMESTAMPS (64x720 gray on stdin)\n",stderr); return 2; }
    times=fopen(argv[1],"r"); if(times==NULL) return 2;
    occ_cook_assembler_init(&assembler,200000000U);
    occ_tracker_init(&tracker,0x12345678U);
    while((scan=fscanf(times,"%" SCNu64,&capture))==1) {
        if(++frames>10000U || capture<80000U || (frames>1U && capture<=last) ||
           capture>UINT64_MAX-720U*40000U-5000U ||
           fread(frame,1U,sizeof(frame),stdin)!=sizeof(frame)) {
            fclose(times); return 2;
        }
        last=capture;
        rc=occ_cook_decode_image(&assembler,&image,&roi,&thresholds,&profile,
                                capture,capture-80000U,&packet,&image_uncertain);
        if(rc<0) { fclose(times); return 2; }
        uncertain+=image_uncertain;
        if(rc==1 && occ_tracker_accept(&tracker,capture,&packet,&progress)==OCC_OK)
            packets++;
    }
    if(scan!=EOF || fgetc(stdin)!=EOF) { fclose(times); return 2; }
    printf("{\"frames\":%u,\"packets\":%u,\"uncertain\":%u}\n",frames,packets,uncertain);
    fclose(times); return 0;
}
