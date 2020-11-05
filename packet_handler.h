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

#ifndef _PACKETHANDLER_H_
#define _PACKETHANDLER_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <time.h>
#include "datastructs.h"
#include "time_calc.h"

//#define SO_TXTIME 63
//#define SCM_TXTIME SO_TXTIME
/* Defines for configurable header values */

#define WGRPID 0x1000
#define GRPVER 0x26DEFA00       //seconds since Jan 1,2000, creation Sept 1,2020
#define MAXPKTSZ 1500 

#define WRITERID_CNTRL 0xAC00
#define WRITERID_AXX 0xAC01
#define WRITERID_AXY 0xAC02
#define WRITERID_AXZ 0xAC03
#define WRITERID_AXS 0xAC04

#define DSTADDRCNTRL "01:AC:CE:55:00:00"    //Ethernet Multicast-Address the Control should sent to
#define DSTADDRAXSX  "01:AC:CE:55:00:01"     //Ethernet Multicast-Address x-Axis should sent to
#define DSTADDRAXSY  "01:AC:CE:55:00:02"     //Ethernet Multicast-Address y-Axis should sent to
#define DSTADDRAXSZ  "01:AC:CE:55:00:03"     //Ethernet Multicast-Address z-Axis should sent to
#define DSTADDRAXSS  "01:AC:CE:55:00:04"     //Ethernet Multicast-Address the Spindle should sent to

#define ETHERTYPE 0xB62C        //Ethertype for OPC UA UADP NetworkMessages over Ethernet II

/* static defines */
#define DBLOVERFLOW INT64_MAX*1e-9


/* struct definitions for networkpacket structure */
#pragma pack(push)      /* push current alignment to stack */
#pragma pack(1)         /* set alignment to 1 byte boundary */

struct eth_hdr_t {
        char dstmac[6];
        char srcmac[6];
        uint16_t ethtyp;
};
/*
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
*/
struct ntwrkmsg_hdr_t {
        uint8_t ver_fl;
        uint8_t extfl;
        uint16_t pubId;
};

struct grp_hdr_t {
        uint8_t grpfl;
        uint16_t wgrpId;
        uint32_t grpVer;
        //uint16_t ntwrkMsgNo;          //removed in demo
        uint16_t seqNo;
};

struct pyld_hdr_t {
        uint8_t msgcnt;
        uint16_t wrtrId;
};

struct extntwrkmsg_hdr_t {
        uint64_t timestamp;
};

struct szrry_t {
        uint16_t size;          //this is an array
};

struct dtstmsg_cntrl_t{
        uint8_t dtstmsg_hdr;            //only DataSetFlags1
        uint16_t fldcnt;                //should be 11
        int64_t xvel_set;		//double encoded as Int64
	int64_t yvel_set;		//double encoded as Int64
	int64_t zvel_set;		//double encoded as Int64
	int64_t spindlespeed;		//double encoded as Int64
	uint8_t xenable;                //bool encoded as uint8			
	uint8_t yenable;		//bool encoded as uint8		
	uint8_t zenable;		//bool encoded as uint8		
	uint8_t spindleenable;		//bool encoded as uint8	
	uint8_t spindlebrake;		//bool encoded as uint8	
	uint8_t machinestatus;		//bool encoded as uint8	
	uint8_t estopstatus;            //bool encoded as uint8	
};

struct dtstmsg_axs_t {
        uint8_t dtstmsg_hdr;            //only DataSetFlags1
        uint16_t fldcnt;                //should be 2 for axis
        uint64_t pos_cur;               //double encoded as uInt64
        uint8_t fault;                  //bool encoded as uint8
};

#pragma pack(pop)       /* reset to original alignment */

union dtstmsg_t {
        struct dtstmsg_cntrl_t dtstmsg_cntrl;
        struct dtstmsg_axs_t dtstmsg_axs;
};

/* struct definition for network packet */
struct rt_pkt_t {
        unsigned char *sktbf;
        struct eth_hdr_t *eth_hdr;
        struct ip_hdr_t *ip_hdr;
        struct udp_hdr_t *udp_hdr;
        struct ntwrkmsg_hdr_t *ntwrkmsg_hdr;
        struct grp_hdr_t *grp_hdr;
        struct pyld_hdr_t *pyld_hdr;
        struct extntwrkmsg_hdr_t *extntwrkmsg_hdr;
        struct szrry_t *szrry;
        union dtstmsg_t *dtstmsg;
        uint32_t len;
};

/* Enum for Type of the DataSetMessage */
enum msgtyp_t {
        CNTRL,
        AXS,
};

/* Enum for variable Type, see variable list in "mk_shminterface.h" */
/*
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
*/



/* allocates a new packet and buffer  */
int createpkt(struct rt_pkt_t** pkt);

/* clears the paket and frees the memory */
void destroypkt(struct rt_pkt_t* pkt);

/* sets a packet, clears the buffer, sets the pointers according to msgcnt
 * and msgtype, then inits paket headers 
 * Limitation in implementation: Cannot mix messages of mutliple types */
int setpkt(struct rt_pkt_t* pkt, int msgcnt, enum msgtyp_t msgtyp);

/* sets all standard header values which do not change during the program
 * execution */
void initpkthdrs(struct rt_pkt_t* pkt);

/* fill packet with information from control, packet must already have the
 * correct number of message (1)*/
int fillcntrlpkt(struct rt_pkt_t* pkt, struct cntrlnfo_t* cntrlnfo, uint16_t seqno);

/* fill packet with information from axs, packet must already have the
 * correct number of message (1)*/
int fillaxspkt(struct rt_pkt_t* pkt, struct axsnfo_t* axsnfo, uint16_t seqno);

/* converts double to int64 by changing the unit to nano units
 * e.g. multiplying by 10^9, keeping the sign */
int dbl2nint64(double val, int64_t *res);

/* converts int64 to double by changing the nano unit to units
 * e.g. multiplying by 10^-9, keeping the sign */
double nint642dbl(int64_t val);

/* fills the LinkLayer(Ethernet)-Address structure */
int fillethaddr(struct sockaddr_ll *addr, uint8_t *mac_addr, uint16_t ethtyp, int fd, char *ifnm);

/* fills the msghdr structure with the necessary values to send the packet */
int fillmsghdr(struct msghdr *msg_hdr, struct sockaddr_ll *addr, uint64_t txtime, clockid_t clkid);

/* sends the packet with the specified txtime and other values */
int sendpkt(int fd, void *buf, int buflen, struct msghdr *msg_hdr);

/* open receive socket as a RAW-packet socket with with AF_PACKET. 
 * Activate reception of Ethernet-Multicast-packets for specifies addresses
 * and bind to specified interface. Returns socket ID.
 */
int opnrxsckt(char *ifnm, char *mac_addrs[], int no_macs);

/* receives a packet, from socket */
int rcvpkt(int fd, struct rt_pkt_t* pkt, struct msghdr * rcvmsg_hdr);         //TODO check how memory managemeant is done here

/* checks the eth_hdr for one of the requested multicast-adresses
 * and the correct ethertype directly using the pkt_skb_buffer
 */
int chckethhdr(struct rt_pkt_t* pkt, char *mac_addrs[], int no_macs);

/* checks the msghdr structure for the correct destination address */
int chckmsghdr(struct msghdr *msg_hdr, char *mac_addr, uint16_t ethtyp);

/* parses packet and sets the pointers correctly returns the messagecount and msgtype */
int prspkt(struct rt_pkt_t* pkt, enum msgtyp_t *msgtyp);

/* checks paket headers for correct values like flags, groupids and so on */
int chckpkthdrs(struct rt_pkt_t* pkt);

/* parse dataset messages from packet, return datasetmessages 
 * limitation: only packets with a single type of dataset-message is supported
 */
int prsdtstmsg(struct rt_pkt_t* pkt, enum msgtyp_t pkttyp, union dtstmsg_t *dtstmsgs[], int *dtstmsgcnt);

/* parse axis information from datasetmessage */
int prsaxsmsg(union dtstmsg_t *dtstmsg, struct axsnfo_t * axsnfo);

/* parse control information from datasetmessage */
int prscntrlmsg(union dtstmsg_t *dtstmsg, struct cntrlnfo_t * cntrlnfo);


/* ##### PacketStore ###### */
/* Holds and manages pointers to allocated packets to manage memory */

/* Packetstorelement, has pointer to packet as well as flag if used or not */
struct pktstrelmt_t {
        struct rt_pkt_t* pkt;
        bool used;
};

/* Packetstorage */
struct pktstore_t {
        struct pktstrelmt_t* pktstrelmt;
        uint32_t size;
};

/* allocates necaessary memory for Packetstorage, elements and packets */
int initpktstrg(struct pktstore_t *pktstore, uint32_t size);

/* frees all packets and frees memory from packetstoreage, regardless if packets
 * are used */
int destroypktstrg(struct pktstore_t  *pktstore);

/* gets an unused pkt from packetstore and marks it as used */
int getfreepkt(struct pktstore_t *pktstore, struct rt_pkt_t** pkt);

/* returns used pkt to packetsore and marks it as free */
int retusedpkt(struct pktstore_t *pktstore, struct rt_pkt_t** pkt);

/* ###### END PacketStore ##### */


#endif /* _PACKETHANDLER_H_ */
