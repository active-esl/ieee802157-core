/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline finite raw-grayscale adapter; no camera or device APIs. */
#include "occ/image.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
static int number(const char *s,uint64_t *v)
{
    char *end; unsigned long long n;
    if(s==NULL || s[0]=='-' || s[0]=='+') return 0;
    errno=0; n=strtoull(s,&end,0);
    if(errno || end==s || *end!='\0') return 0;
    *v=(uint64_t)n; return 1;
}
int main(int argc,char **argv)
{
    uint64_t args[9],time,last=0U;
    size_t pixels;
    unsigned frames=0U,packets=0U,uncertain=0U,rejected=0U,i;
    uint8_t *buffer;
    FILE *times;
    int scan,sample,rc,progress;
    occ_packet packet={0};
    occ_receiver receiver;
    occ_tracker tracker;
    occ_gray_image image;
    occ_roi roi;
    const occ_thresholds thresholds={70U,180U,100U,150U,2U,253U,250U};
    if(argc!=11) {
        fputs("usage: occ_decode_gray WIDTH HEIGHT X Y W H TIMESTAMPS PERIOD_NS TOLERANCE_NS EXPECTED_BOARD\n",stderr);
        return 2;
    }
    for(i=0U;i<6U;++i) if(!number(argv[i+1U],&args[i])) return 2;
    for(i=6U;i<9U;++i) if(!number(argv[i+2U],&args[i])) return 2;
    if(args[0]==0U || args[1]==0U || args[0]>1024U || args[1]>1024U ||
       args[8]>UINT32_MAX) return 2;
    pixels=(size_t)(args[0]*args[1]);
    buffer=malloc(pixels); if(buffer==NULL) return 2;
    times=fopen(argv[7],"r"); if(times==NULL) { free(buffer); return 2; }
    image.data=buffer; image.width=(size_t)args[0]; image.height=(size_t)args[1];
    image.stride=image.width;
    roi.x=(size_t)args[2]; roi.y=(size_t)args[3];
    roi.width=(size_t)args[4]; roi.height=(size_t)args[5];
    if(occ_receiver_init(&receiver,OCC_RX_UFSOOK,args[6],args[7])!=OCC_OK) {
        fclose(times); free(buffer); return 2;
    }
    occ_tracker_init(&tracker,(uint32_t)args[8]);
    while((scan=fscanf(times,"%" SCNu64,&time))==1) {
        if(++frames>10000U || fread(buffer,1U,pixels,stdin)!=pixels ||
           occ_gray_classify(&image,&roi,&thresholds,&sample)!=OCC_OK) {
            fclose(times); free(buffer); return 2;
        }
        rc=occ_receiver_push(&receiver,time,sample,&packet);
        last=time;
        if(rc==OCC_PACKET_READY) {
            if(occ_tracker_accept(&tracker,time,&packet,&progress)==OCC_OK) packets++;
            else rejected++;
        } else if(rc<0) uncertain++;
    }
    if(scan!=EOF || fgetc(stdin)!=EOF) { fclose(times); free(buffer); return 2; }
    printf("{\"frames\":%u,\"packets\":%u,\"uncertain\":%u,\"rejected_source\":%u,\"last_time_ns\":%" PRIu64 ",\"sequence\":%u}\n",
        frames,packets,uncertain,rejected,last,(unsigned)packet.sequence);
    fclose(times); free(buffer); return 0;
}
