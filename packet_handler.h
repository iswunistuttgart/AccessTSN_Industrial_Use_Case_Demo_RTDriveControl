// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/*
 * This uses OPC UA Networkmessages to communicate the variables and values.
 * Each variable is decoded into a "DataSetMessage" with two fields. The first
 * field is the variable-ID [4 Byte] and the second field is the value [8 byte].
 * A Networkmessage can contain multiple DataSrtMessages and therefore multiple
 * variables. The variable-ID is defined in this document.
 */

#include <stdint.h>
#include <string.h>

/* Defines for configurable header values */

#define WGRPID 0x1000
#define GRPVER 0x26DEFA00       //seconds since Jan 1, 2000, creation Sept 1,2020      


/* struct definitions for networkpacket structure */
#pragma pack(push)      /* push current alignment to stack */
#pragma pack(1)         /* set alignment to 1 byte boundary */

struct eth_hdr_t {
        uint8_t dstmac[6];
        uint8_t srcmac[6];
        uint16_t ethtyp;
};

struct ip_hdr_t {
        uint8_t vl;
        uint8_t dsf;
        uint16_t len;
        uint16_t id;
        uint16_t flag_fragoff;
        uint8_t ttl;
        uint8_t proto;
        uint16_t hdr_chksm;
        uint32_t srcaddr;
        uint32_t dstaddr;
};

struct udp_hdr_t {
        uint16_t srcp;
        uint16_t dstp;
        uint16_t len;
        uint16_t chksm;
};

struct ntwrkmsg_hdr_t {
        uint8_t ver_fl;
        uint8_t extfl;
        uint16_t pubId;
};

struct grp_hdr_t {
        uint8_t grpfl;
        uint16_t wgrpId;
        uint32_t grpVer;
        uint16_t ntwrkMsgNo;
        uint16_t seqNo;
};

struct pyld_hdr_t {
        uint8_t msgcnt;
        uint16_t * wrtrId;
};

struct extntwrkmsg_hdr_t {
        uint64_t timestamp;
};

struct dtstmsg_t {
        uint32_t varId;
        uint64_t value;
};

#pragma pack(pop)       /* reset to original alignment */

/* struct definition for network packet */
struct rt_pkt_t {
        unsigned char *sktbf;
        struct eth_hdr_t *eth_hdr;
        struct ip_hdr_t *ip_hdr;
        struct udp_hdr_t *udp_hdr;
        struct ntwrkmsg_hdr_t *ntwrkmsg_hdr;
        struct grp_hdr_t *grp_dhr;
        struct pyld_hdr_t *pylhdr;
        struct extntwrkmsg_hdr_t *extntwrkmsg_hdr;
        struct dtstmsg_t *dtstmsg_t;
        uint32_t len;
};

/* Enum for variable Type, see variable list in "mk_shminterface.h" */
enum varID_t {
        xvel_set = 0,
	yvel_set = 1,
        zvel_set = 2,
        spindlespeed = 3,
        xenable = 4,
        yenable = 5,
        zenable = 6,
        spindleenable = 7,
        spindlebrake = 8,
        machinestatus = 9,
        estopstatus = 10,
        feedrate = 11,
        feedoverride = 12,
        xpos_set = 13,
        ypos_set = 14,
        zpos_set = 15,
        lineno = 16,
        uptime = 17,
        tool = 18,
        mode = 19,
        xhome = 20,
        yhome = 21,
        zhome = 22,
        xhardneg = 23,
        xhardpos = 24,
        yhardneg = 25,
        yhardpos = 26,
        zhardneg = 27,
        zhardpos = 28,
        xpos_cur = 29,
        ypos_cur = 30,
        zpos_cur = 31,
        xfault = 32,
        yfault = 33,
        zfault = 34,
};

/* allocates a new packet and buffer and sets the packet */
int createpkt(struct rt_pkt_t* pkt, int msgcnt);

/* clears the paket and frees the memory */
int destroypkt(struct rt_pkt_t*pkt);

/* resets a packet, clears the buffer, sets the pointers according to msgcnt,
inits paket headers */
void resetpkt(struct rt_pkt_t* pkt, int msgcnt);

/* sets all standard header values with do not change during the program
execution */
void Initpkthdrs(struct rt_pkt_t* pkt);