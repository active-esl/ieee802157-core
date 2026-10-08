/* SPDX-License-Identifier: GPL-3.0-only */
/* Finite offline decoder-to-status adapter. No camera or device access.
 * Host test records: monotonic_ns + tick/uncertain/44hex envelope.
 * Not an optical wire format and never a command/authority channel. */
#include "occ/packet.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int number(const char *s,uint64_t *out)
{
    const char *p; char *end; unsigned long long n;
    if(s==NULL || *s=='\0') return 0;
    for(p=s;*p!='\0';++p) if(*p<'0' || *p>'9') return 0;
    errno=0; n=strtoull(s,&end,10);
    if(errno || *end!='\0') return 0;
    *out=(uint64_t)n; return 1;
}
static int hex(char c)
{
    if(c>='0' && c<='9') return c-'0';
    if(c>='a' && c<='f') return c-'a'+10;
    if(c>='A' && c<='F') return c-'A'+10;
    return -1;
}
static int envelope(const char *s,uint8_t bytes[OCC_PACKET_BYTES])
{
    size_t i; int a,b;
    if(strlen(s)!=OCC_PACKET_BYTES*2U) return 0;
    for(i=0U;i<OCC_PACKET_BYTES;++i) {
        a=hex(s[i*2U]); b=hex(s[i*2U+1U]);
        if(a<0 || b<0) return 0;
        bytes[i]=(uint8_t)((unsigned)a*16U+(unsigned)b);
    }
    return 1;
}
int main(int argc,char **argv)
{
    static const char *names[]={"no_signal","uncertain","valid","stale"};
    uint64_t board,signal_timeout,progress_timeout,now,last=0U;
    unsigned count=0U; char line[128],stamp[32],event[48],extra;
    uint8_t bytes[OCC_PACKET_BYTES]; occ_packet packet;
    occ_tracker tracker; int accepted,progress;
    occ_observation_state state;
    if(argc!=4 || !number(argv[1],&board) || board>UINT32_MAX ||
       !number(argv[2],&signal_timeout) || signal_timeout==0U ||
       !number(argv[3],&progress_timeout) || progress_timeout==0U) {
        fputs("usage: occ_status_stream EXPECTED_BOARD SIGNAL_TIMEOUT_NS PROGRESS_TIMEOUT_NS\n",stderr);
        return 2;
    }
    if(setvbuf(stdout,NULL,_IOLBF,0)!=0) return 2;
    occ_tracker_init(&tracker,(uint32_t)board);
    while(fgets(line,sizeof(line),stdin)!=NULL) {
        if(++count>10000U || strchr(line,'\n')==NULL ||
           sscanf(line,"%31s %47s %c",stamp,event,&extra)!=2 ||
           !number(stamp,&now) || (count>1U && now<=last)) return 2;
        last=now; accepted=0; progress=0;
        if(strcmp(event,"tick")==0) {
            /* Independent host timeout tick; no fabricated optical sample. */
        } else if(strcmp(event,"uncertain")==0) {
            if(occ_observe(&tracker.observation,now,1,0,0)!=OCC_OK) return 2;
        } else {
            if(!envelope(event,bytes)) return 2;
            if(occ_packet_decode(bytes,&packet)==OCC_OK &&
               occ_tracker_accept(&tracker,now,&packet,&progress)==OCC_OK)
                accepted=1;
            else if(occ_observe(&tracker.observation,now,1,0,0)!=OCC_OK) return 2;
        }
        state=occ_observation_status(&tracker.observation,now,
                                    signal_timeout,progress_timeout);
        printf("{\"time_ns\":%" PRIu64 ",\"state\":\"%s\","
               "\"accepted\":%s,\"progressed\":%s,\"authenticated\":false,"
               "\"authoritative\":false,\"expected_board\":%" PRIu64 ",\"packet\":",
               now,names[state],accepted?"true":"false",progress?"true":"false",board);
        if(accepted) {
            printf("{\"board\":%" PRIu32 ",\"session\":%" PRIu32 ","
                   "\"sequence\":%u,\"kind\":%u,\"status\":%u,\"build\":%" PRIu32 "}}\n",
                   packet.board,packet.session,(unsigned)packet.sequence,
                   (unsigned)packet.kind,(unsigned)packet.status,packet.build);
        } else puts("null}");
    }
    return ferror(stdin) || fflush(stdout)!=0 || ferror(stdout) ? 2 : 0;
}
