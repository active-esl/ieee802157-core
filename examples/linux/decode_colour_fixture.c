/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline RGB SYMBOL fixture adapter. No capture or blind clock acquisition.
 * Input is recovered symbol centre_ns/R/G/B; -1/-1/-1 represents an erasure.
 * Calibration below is SYNTHETIC, never a physical camera default. */
#include "occ/colour.h"
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
static int read_line(char *line,size_t capacity)
{
    size_t n=0;
    int ch;
    while((ch=fgetc(stdin))!=EOF) {
        if(ch==0 || n+1U>=capacity) return -1;
        line[n++]=(char)ch;
        if(ch=='\n') break;
    }
    line[n]='\0';
    return n ? 1 : 0;
}
static void skip_space(char **p)
{
    while(isspace((unsigned char)**p)) ++*p;
}
/* scanf integer conversion does not safely diagnose overflow. Parse each
 * bounded token explicitly; a negative timestamp must never wrap to uint64. */
static int parse_line(char *line,uint64_t *stamp,int rgb[3])
{
    char *p=line,*end;
    uintmax_t value;
    unsigned i;
    skip_space(&p);
    if(!isdigit((unsigned char)*p)) return 0;
    errno=0; value=strtoumax(p,&end,10);
    if(errno || end==p || value>UINT64_MAX || !isspace((unsigned char)*end)) return 0;
    *stamp=(uint64_t)value; p=end;
    for(i=0;i<3U;++i) {
        long colour;
        skip_space(&p);
        if(*p!='-' && !isdigit((unsigned char)*p)) return 0;
        errno=0; colour=strtol(p,&end,10);
        if(errno || end==p || colour < -1 || colour>255 ||
           (*end && !isspace((unsigned char)*end))) return 0;
        rgb[i]=(int)colour; p=end;
    }
    skip_space(&p);
    return !*p;
}
int main(void)
{
    const occ_colour_calibration c={{5,5,5},
        {{100,5,3},{4,100,7},{5,10,100},{80,85,75}},40,250,400,200};
    occ_colour_receiver rx;
    occ_tracker tracker;
    occ_packet packet;
    unsigned samples=0,packets=0,uncertain=0,rejected=0;
    char line[160];
    uint64_t time_ns;
    int values[3],r,g,b,symbol,rc,progress,line_status;
    if(occ_colour_receiver_init(&rx,125000000U,10000000U)!=OCC_OK) return 2;
    occ_tracker_init(&tracker,42);
    while((line_status=read_line(line,sizeof(line)))>0) {
        uint16_t rgb[3];
        if(++samples>10000U ||
           !parse_line(line,&time_ns,values)) return 2;
        r=values[0]; g=values[1]; b=values[2];
        symbol=-1;
        if(r!=-1 || g!=-1 || b!=-1) {
            if(r<0 || g<0 || b<0 || r>255 || g>255 || b>255) return 2;
            rgb[0]=(uint16_t)r; rgb[1]=(uint16_t)g; rgb[2]=(uint16_t)b;
            if(occ_colour_classify(&c,rgb,&symbol)!=OCC_OK) symbol=-1;
        }
        rc=occ_colour_receiver_push(&rx,time_ns,symbol,&packet);
        if(rc==OCC_PACKET_READY) {
            if(occ_tracker_accept(&tracker,time_ns,&packet,&progress)==OCC_OK) {
                packets++;
                printf("{\"sequence\":%u,\"board\":%" PRIu32 ",\"session\":%" PRIu32
                    ",\"build\":%" PRIu32 ",\"status\":%u,\"kind\":%u,\"time_ns\":%" PRIu64 "}\n",
                    (unsigned)packet.sequence,packet.board,packet.session,packet.build,
                    (unsigned)packet.status,(unsigned)packet.kind,time_ns);
            } else rejected++;
        } else if(rc<0) uncertain++;
    }
    if(line_status<0 || ferror(stdin)) return 2;
    fprintf(stderr,"samples=%u packets=%u uncertain=%u rejected_source=%u\n",
        samples,packets,uncertain,rejected);
    return 0;
}
