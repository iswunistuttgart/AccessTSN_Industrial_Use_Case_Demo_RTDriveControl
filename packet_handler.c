// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "packet_handler.h"
#include <arpa/inet.h>

void initpkthdrs(struct rt_pkt_t* pkt)
{
        // eth_hdr
        // pkt->eth_hdr->ethtyp = htons(ETHERTYPE);
        // ntwrkmsg_hdr_t 
        pkt->ntwrkmsg_hdr->ver_fl = 0xF1;       //Version = 1; PublishID, GroupHeader, PayloadHdr and ExtendedFlags 1 enables
        pkt->ntwrkmsg_hdr->extfl = 0x21;        //pubishID datatype Uint16; Timestamp enabled
        //TODO pkt.ntwrkmsg_hdr->pubId;
        // grp_hdr_t
        pkt->grp_hdr->grpfl = 0x0B;             //writergroupID, groupVersion and Sequencenumber enabled
        pkt->grp_hdr->wgrpId = WGRPID;
        pkt->grp_hdr->grpVer = GRPVER;
}

int setpkt(struct rt_pkt_t* pkt, int msgcnt, enum msgtyp_t msgtyp)
{
        uint16_t dtstmsgsz = 0;
        int msgfldsz;
        int msgfldcnt;
        memset(pkt->sktbf,0,MAXPKTSZ*sizeof(char));
        pkt->eth_hdr = NULL; //pkt->sktbf;
        pkt->ip_hdr = NULL;
        pkt->udp_hdr = NULL;
        pkt->ntwrkmsg_hdr = (struct ntwrkmsg_hdr_t*) pkt->sktbf;//(struct ntwrkmsg_hdr_t*) ((char *) pkt->eth_hdr + sizeof(struct eth_hdr_t));
        pkt->grp_hdr = (struct grp_hdr_t*) ((char *) pkt->ntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
        pkt->pyld_hdr = (struct pyld_hdr_t*) ((char *) pkt->grp_hdr + sizeof(struct grp_hdr_t));
        pkt->pyld_hdr->msgcnt = msgcnt;
        pkt->extntwrkmsg_hdr = (struct extntwrkmsg_hdr_t*) ((char *) pkt->pyld_hdr + sizeof(pkt->pyld_hdr->msgcnt) + msgcnt*sizeof(/**(pkt->pyld_hdr->wrtrId)*/uint16_t));
        if (msgcnt < 2){
                //for msgcnt = 1, sizearray is ommitted
                pkt->szrry = NULL;
                msgfldsz = 0;
                msgfldcnt = 0;
        } else {
                //for msgcnt >1, set sizes of datasetmessages in sizearray
                pkt->szrry = (struct szrry_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
                msgfldsz = msgcnt*sizeof(*(pkt->szrry));
                msgfldcnt = msgcnt;
        }
        pkt->dtstmsg = (union dtstmsg_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t) + msgfldsz);
        int i = 0;
        switch(msgtyp){
        case CNTRL:
                pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyld_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyld_hdr->wrtrId)) + msgfldsz + msgcnt*sizeof(struct dtstmsg_cntrl_t);
                dtstmsgsz = sizeof(struct dtstmsg_cntrl_t);
                while(i<msgcnt) {
                        pkt->dtstmsg[i].dtstmsg_cntrl.dtstmsg_hdr = 0x01;
                        pkt->dtstmsg[i].dtstmsg_cntrl.fldcnt = 11;
                        i++;
                }
                break;
        case AXS:
                pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyld_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyld_hdr->wrtrId)) + msgfldsz + msgcnt*sizeof(struct dtstmsg_axs_t);
                dtstmsgsz = sizeof(struct dtstmsg_axs_t);
                while(i<msgcnt) {
                        pkt->dtstmsg[i].dtstmsg_axs.dtstmsg_hdr = 0x01;
                        pkt->dtstmsg[i].dtstmsg_axs.fldcnt = 2;
                        i++;
                }
                break;
        default:
                return 1;       //fail
        };
        i = 0;
        while (i <msgfldcnt) {
                *(&(pkt->szrry->size) + i) = dtstmsgsz;
                i++;
        }
        
        
        initpkthdrs(pkt);
        
        return 0;       //succeded
}

int createpkt(struct rt_pkt_t** pkt)
{
        struct rt_pkt_t* newpkt;
        int ok;
        
        newpkt = (struct rt_pkt_t*) calloc(1,sizeof(struct rt_pkt_t));
        if (NULL == newpkt){
                pkt = NULL;
                return 1;       //fail
        }
        //memset(newpkt,0,sizeof(struct rt_pkt_t));

        newpkt->sktbf = (char *) malloc(MAXPKTSZ*sizeof(char));
        if (NULL == newpkt->sktbf) {
                free(newpkt);
                pkt = NULL;     //fail
                return 1;
        }

        *pkt = newpkt;
        return 0;       //succeded
}

void destroypkt(struct rt_pkt_t* pkt)
{
        pkt->eth_hdr = NULL;
        pkt->ip_hdr = NULL;
        pkt->udp_hdr = NULL;
        pkt->ntwrkmsg_hdr = NULL;
        pkt->grp_hdr = NULL;
        pkt->pyld_hdr = NULL;
        pkt->extntwrkmsg_hdr = NULL;
        pkt->dtstmsg = NULL;
        free(pkt->sktbf);
        pkt->sktbf = NULL;
        pkt->len = 0;
        pkt->sktbf = NULL;
        free(pkt);
}

int fillcntrlpkt(struct rt_pkt_t* pkt, struct cntrlnfo_t* cntrlnfo, uint16_t seqno)
{
        int64_t tmp;
        struct timespec time;
        int ok = 0;
        pkt->grp_hdr->seqNo = seqno;
        //check for current limitation that only one control message is supported
        if(pkt->pyld_hdr->msgcnt != 1)
                return 1; //fail
        pkt->pyld_hdr->wrtrId[0] = WRITERID_CNTRL;
        ok += dbl2nint64(cntrlnfo->x_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.xvel_set = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.xenable = (uint8_t) cntrlnfo->x_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->y_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.yvel_set = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.yenable = (uint8_t) cntrlnfo->y_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->z_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.zvel_set = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.zenable = (uint8_t) cntrlnfo->z_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->s_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.spindlespeed = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.spindleenable = (uint8_t) cntrlnfo->s_set.cntrlsw;
        pkt->dtstmsg[0].dtstmsg_cntrl.spindlebrake = (uint8_t) cntrlnfo->spindlebrake;
        pkt->dtstmsg[0].dtstmsg_cntrl.machinestatus = (uint8_t) cntrlnfo->machinestatus;
        pkt->dtstmsg[0].dtstmsg_cntrl.estopstatus = (uint8_t) cntrlnfo->estopstatus;

        clock_gettime(CLOCK_TAI,&time);
        pkt->extntwrkmsg_hdr->timestamp = cnvrt_tmspc2uatm(time);

        return ok;
}

int dbl2nint64(double val, int64_t* res)
{
        if((val > DBLOVERFLOW) || (val < -1*DBLOVERFLOW))
                return 1;       //fail, value to large
        *res = (int64_t)(val*1e9);
        return 0;       //succeded
}

double nint642dbl(int64_t val)
{
        return (double)(val*1e-9);
}

int fillethaddr(struct sockaddr_ll *addr, uint8_t *mac_addr, uint16_t ethtyp, int fd, char *ifnm)
{
        if (NULL == addr)
                return 1;       //fail
        
        // get index of network interface by name
        struct ifreq iface;
        memset(&iface, 0, sizeof(struct ifreq));
        strncpy(iface.ifr_name, ifnm, strnlen(ifnm, IFNAMSIZ));
        if((ioctl(fd,SIOCGIFINDEX,&iface)) == -1)
                return 1;       //fail
        
        memset(addr, 0, sizeof(struct sockaddr_ll));
        addr->sll_family = AF_PACKET;
        addr->sll_protocol = htons(ethtyp);
        addr->sll_halen = ETH_ALEN;
        addr->sll_ifindex = iface.ifr_ifindex;
        memcpy(addr->sll_addr,mac_addr,ETH_ALEN);

        return 0;       //succeded
}

int fillmsghdr(struct msghdr *msg_hdr, struct sockaddr_ll *addr, uint64_t txtime, clockid_t clkid)
{
        struct cmsghdr *cmsg;
        char cntlmsg[CMSG_SPACE(sizeof(txtime)) /*+ CMSG_SPACE(sizeof(clkid)) + CMSG_SPACE(sizeof(uint8_t))*/] = {};
        uint8_t drop_if_late = 1;

        if(NULL == msg_hdr)
                return 1;       //fail
                
        memset(msg_hdr,0,sizeof(struct msghdr));
        msg_hdr->msg_name = addr;
        msg_hdr->msg_namelen = sizeof(struct sockaddr_ll);
        msg_hdr->msg_control = cntlmsg;
        msg_hdr->msg_controllen = sizeof(cntlmsg);

        cmsg = CMSG_FIRSTHDR(msg_hdr);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_TXTIME;
        cmsg->cmsg_len = CMSG_LEN(sizeof(txtime));
        *((uint64_t *) CMSG_DATA(cmsg)) = txtime;

        /* not in lkernel v5.9 or older
        cmsg = CMSG_NXTHDR(msg_hdr, cmsg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_CLOCKID;
	cmsg->cmsg_len = CMSG_LEN(sizeof(clockid_t));
	*((clockid_t *) CMSG_DATA(cmsg)) = clkid;

        cmsg = CMSG_NXTHDR(msg_hdr, cmsg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_DROP_IF_LATE;
        cmsg->cmsg_len = CMSG_LEN(sizeof(uint8_t));
        *((uint8_t *) CMSG_DATA(cmsg)) = drop_if_late;
        */

        return 0;       //succeded
}


int sendpkt(int fd, void *buf, int buflen, struct msghdr *msg_hdr)
{
        int sndcnt;
        struct iovec msg_iov;
        if(NULL == msg_hdr)
                return 1;       //fail

        msg_hdr->msg_iov = &msg_iov;
        msg_hdr->msg_iov->iov_base = buf;
        msg_hdr->msg_iov->iov_len = buflen;
        msg_hdr->msg_iovlen = 1;
        

        sndcnt = sendmsg(fd,msg_hdr,0);
        if (sndcnt < 0)
                return 1;       //fail
        return 0;
}


/* ##### PacketStore ##### */

int initpktstrg(struct pktstore_t *pktstore, uint32_t size)
{
        if(NULL == pktstore)
                return 1;       //fail
        int i = 0;
        int ok;
        struct rt_pkt_t *pkt;
        pktstore->size = size;
        pktstore->pktstrelmt = (struct pktstrelmt_t*) calloc(size,sizeof(struct pktstrelmt_t));
        if (NULL == pktstore->pktstrelmt){
                pktstore->size = 0;
                return 1;       //fail
        }
        while(i < pktstore->size) {
                pkt = NULL;
                ok = createpkt(&pkt);
                if( ok != 0){
                        pktstore->size = i;
                        return 1;       //fail
                }
                pktstore->pktstrelmt[i].pkt = pkt;
                pktstore->pktstrelmt[i].used = false;
                i++;
        }

        return 0;
}

int destroypktstrg(struct pktstore_t  *pktstore)
{
        if(NULL == pktstore)
                return 1;       //fail

        int i = 0;
        while(i < pktstore->size) {
                destroypkt(pktstore->pktstrelmt[i].pkt);
                pktstore->pktstrelmt[i].pkt = NULL;
                i++;
        }
        pktstore->size = 0;
        free(pktstore->pktstrelmt);
        pktstore->pktstrelmt = NULL;
        return 0;
}

int getfreepkt(struct pktstore_t *pktstore, struct rt_pkt_t** pkt)
{
        if(NULL == pktstore)
                return 1;       //fail
        *pkt = NULL;
        int i = 0;
        while(i < pktstore->size) {
                if(pktstore->pktstrelmt[i].used == false){
                        *pkt = pktstore->pktstrelmt[i].pkt;
                        pktstore->pktstrelmt[i].used = true;
                        i = pktstore->size;
                } else {
                        i++;
                }
                
        }
        if(NULL == *pkt)
                return 1;       //fail
        return 0;       //succeded
}

int retusedpkt(struct pktstore_t *pktstore, struct rt_pkt_t** pkt)
{
        if((NULL == pktstore) || (NULL == pkt))
                return 1;       //fail
        int i = 0;
        while(i < pktstore->size) {
                if(pktstore->pktstrelmt[i].pkt == *pkt){
                        pktstore->pktstrelmt[i].used = false;
                        i = pktstore->size;
                        *pkt = NULL;
                } else {
                        i++;
                }
        }
        if(NULL != *pkt)
                return 1;       //fail
        return 1;       //succeded
}

/* ##### END PacketStore ##### */
