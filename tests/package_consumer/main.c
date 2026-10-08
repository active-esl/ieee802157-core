/* SPDX-License-Identifier: GPL-3.0-only */
#include <occ/packet.h>
int main(void)
{
    const occ_packet input={2U,42U,99U,1U,0U,1234U};
    occ_packet output;
    unsigned char bytes[OCC_PACKET_BYTES];
    return occ_packet_encode(&input,bytes)!=OCC_OK ||
           occ_packet_decode(bytes,&output)!=OCC_OK || output.board!=42U;
}
