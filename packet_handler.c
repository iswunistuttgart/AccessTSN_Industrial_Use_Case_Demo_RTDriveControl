// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "packet_handler.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

void initpkthdrs(struct rt_pkt_t* pkt)
{
        pkt->ntwrkmsg_hdr->ver_fl = 0xF1;       //Version = 1; PublishID, GroupHeader, PayloadHdr and ExtendedFlags 1 enables
        pkt->ntwrkmsg_hdr->extfl = 0x21;        //pubishID datatype Uint16; Timestamp enabled
        //TODO pkt.ntwrkmsg_hdr->pubId;
        // grp_hdr_t
        pkt->grp_hdr->grpfl = 0x0B;             //writergroupID, groupVersion and Sequencenumber enabled
        pkt->grp_hdr->wgrpId = htons(WGRPID);
        pkt->grp_hdr->grpVer = htonl(GRPVER);
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
        pkt->dtstmsg = (union dtstmsg_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct extntwrkmsg_hdr_t) + msgfldsz);
        int i = 0;
        switch(msgtyp){
        case CNTRL:
                pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyld_hdr->msgcnt) + msgcnt*sizeof(pkt->pyld_hdr->wrtrId) + msgfldsz + msgcnt*sizeof(struct dtstmsg_cntrl_t);
                dtstmsgsz = sizeof(struct dtstmsg_cntrl_t);
                while(i<msgcnt) {
                        pkt->dtstmsg[i].dtstmsg_cntrl.dtstmsg_hdr = 0x01;
                        pkt->dtstmsg[i].dtstmsg_cntrl.fldcnt = htons(11);
                        i++;
                }
                break;
        case AXS:
                pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyld_hdr->msgcnt) + msgcnt*sizeof(pkt->pyld_hdr->wrtrId) + msgfldsz + msgcnt*sizeof(struct dtstmsg_axs_t);
                dtstmsgsz = sizeof(struct dtstmsg_axs_t);
                while(i<msgcnt) {
                        pkt->dtstmsg[i].dtstmsg_axs.dtstmsg_hdr = 0x01;
                        pkt->dtstmsg[i].dtstmsg_axs.fldcnt = htons(2);
                        i++;
                }
                break;
        default:
                return 1;       //fail
        };
        i = 0;
        while (i <msgfldcnt) {
                *(&(pkt->szrry->size) + i) = htons(dtstmsgsz);
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
        pkt->grp_hdr->seqNo = htons(seqno);
        //check for current limitation that only one control message is supported
        if(pkt->pyld_hdr->msgcnt != 1)
                return 1; //fail
        pkt->pyld_hdr->wrtrId = htons(WRITERID_CNTRL);
        ok += dbl2nint64(cntrlnfo->x_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.xvel_set = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.xenable = cntrlnfo->x_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->y_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.yvel_set = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.yenable = cntrlnfo->y_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->z_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.zvel_set = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.zenable = cntrlnfo->z_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->s_set.cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.spindlespeed = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_cntrl.spindleenable = cntrlnfo->s_set.cntrlsw;
        pkt->dtstmsg[0].dtstmsg_cntrl.spindlebrake = (uint8_t) cntrlnfo->spindlebrake;
        pkt->dtstmsg[0].dtstmsg_cntrl.machinestatus = (uint8_t) cntrlnfo->machinestatus;
        pkt->dtstmsg[0].dtstmsg_cntrl.estopstatus = (uint8_t) cntrlnfo->estopstatus;

        clock_gettime(CLOCK_TAI,&time);
        pkt->extntwrkmsg_hdr->timestamp = cnvrt_tmspc2uatm(time);

        return ok;
}
 
int fillaxspkt(struct rt_pkt_t* pkt, struct axsnfo_t* axsnfo, uint16_t seqno)
{
        int64_t tmp;
        struct timespec time;
        int ok = 0;
        pkt->grp_hdr->seqNo = htons(seqno);
        
        if(pkt->pyld_hdr->msgcnt != 1)
                return 1; //fail
        switch(axsnfo->axsID){
        case x:
                pkt->pyld_hdr->wrtrId = htons(WRITERID_AXX);
                break;
        case y: 
                pkt->pyld_hdr->wrtrId = htons(WRITERID_AXY);
                break;
        case z:
                pkt->pyld_hdr->wrtrId = htons(WRITERID_AXZ);
                break;
        case s: 
                pkt->pyld_hdr->wrtrId = htons(WRITERID_AXS);
                break;
        default:
                pkt->pyld_hdr->wrtrId = htons(0);
        }
        ok += dbl2nint64(axsnfo->cntrlvl,&tmp);
        pkt->dtstmsg[0].dtstmsg_axs.pos_cur = htobe64(tmp);
        pkt->dtstmsg[0].dtstmsg_axs.fault = (uint8_t) axsnfo->cntrlsw;

        clock_gettime(CLOCK_TAI,&time);
        pkt->extntwrkmsg_hdr->timestamp = cnvrt_tmspc2uatm(time);
        return 0;
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

int sendpkt(int fd, void *buf, int buflen, struct sockaddr_ll *addr, uint64_t txtime, clockid_t clkid)
{
        int sndcnt;
        struct msghdr msg_hdr;
        struct cmsghdr *cmsg;
        char cntlmsg[CMSG_SPACE(sizeof(txtime)) /*+ CMSG_SPACE(sizeof(clkid)) + CMSG_SPACE(sizeof(uint8_t))*/] = {};
        struct iovec msg_iov;
        
        if(NULL == addr)
                return 1;       //fail

        memset(&msg_hdr,0,sizeof(struct msghdr));
        msg_hdr.msg_name = addr;
        msg_hdr.msg_namelen = sizeof(struct sockaddr_ll);
        msg_hdr.msg_control = cntlmsg;
        msg_hdr.msg_controllen = sizeof(cntlmsg);

        msg_hdr.msg_iov = &msg_iov;
        msg_hdr.msg_iov->iov_base = buf;
        msg_hdr.msg_iov->iov_len = buflen;
        msg_hdr.msg_iovlen = 1;

        cmsg = CMSG_FIRSTHDR(&msg_hdr);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_TXTIME;
        cmsg->cmsg_len = CMSG_LEN(sizeof(txtime));
        *((uint64_t *) CMSG_DATA(cmsg)) = txtime;

        /* not in kernel v5.9 or older
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

        sndcnt = sendmsg(fd,&msg_hdr,0);
        if (sndcnt < 0) {
		printf("error in sndmsg, errono: %d;",errno);
                return 1;       //fail
	}
        return 0;
}

int opnrxsckt(char *ifnm, char *mac_addrs[], int no_macs)
{
        struct ifreq ifopts;
        int scktopts;
        int rxsckt;
        struct sockaddr_ll scktaddr;
        struct packet_mreq pkt_mr;
        memset(&pkt_mr, 0, sizeof(struct packet_mreq));

        rxsckt = socket(AF_PACKET,SOCK_RAW,htons(ETHERTYPE));
        if (rxsckt < 0)
                return -1;      //fail

        // allow socket to be reused
        if (setsockopt(rxsckt, SOL_SOCKET, SO_REUSEADDR, &scktopts, sizeof(scktopts)) < 0) {
                close(rxsckt);
                return -1;      //fail
        }

        //get interface ID
        strncpy(ifopts.ifr_name, ifnm, IFNAMSIZ);
        if (ioctl(rxsckt, SIOCGIFINDEX, &ifopts) < 0) {
                close(rxsckt);
                return -1;
        }
        
        // set interface to get multicast macs
        pkt_mr.mr_ifindex = ifopts.ifr_ifindex;
        pkt_mr.mr_type = PACKET_MR_MULTICAST;
        pkt_mr.mr_alen = ETH_ALEN;
        char * test;
        for (int i = 0; i < no_macs; i++) {
                memcpy(pkt_mr.mr_address, mac_addrs[i],ETH_ALEN);
                if (setsockopt(rxsckt, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &pkt_mr,sizeof(struct packet_mreq)) < 0) {
                        close(rxsckt);
                        return -1;      //fail
                }
        }
        
        return rxsckt;
}

int rcvpkt(int fd, struct rt_pkt_t* pkt, struct msghdr * rcvmsg_hdr)
{
        int ok = 0;
        struct iovec msg_iov;
        if (NULL == pkt)
                return 1;       //fail
        if(NULL == rcvmsg_hdr)
                return 1;       //fail

        memset(rcvmsg_hdr,0,sizeof(struct msghdr));
        rcvmsg_hdr->msg_iov = &msg_iov;
        rcvmsg_hdr->msg_iov->iov_base = pkt->sktbf;
        rcvmsg_hdr->msg_iov->iov_len = MAXPKTSZ;
        rcvmsg_hdr->msg_iovlen = 1;
        
        ok = recvmsg(fd, rcvmsg_hdr, MSG_DONTWAIT);
        if(ok < 0){
                printf("recv failed errno: %d\n",errno);
                return 1;       //fail
        }
        if(MSG_TRUNC == (rcvmsg_hdr->msg_flags & MSG_TRUNC))
                return 1;       //fail
        pkt->len = ok;

        printf("Recvd Packet in recv fnkt: ");
        for (int i = 0 ;i<100;i++)
                printf("%02x",*((uint8_t *)(msg_iov.iov_base+i)));
        printf("\n");

        return 0;
}

int chckethhdr(struct rt_pkt_t* pkt, char *mac_addrs[], int no_macs)
{
        struct eth_hdr_t * rcvd_ethhdr;
        rcvd_ethhdr = (struct eth_hdr_t *) pkt->sktbf;
        if (ETHERTYPE != ntohs(rcvd_ethhdr->ethtyp))
                return -1;       //fail
        
        for (int i = 0; i<no_macs; i++) {
                if(memcmp((rcvd_ethhdr->dstmac), mac_addrs[i], ETH_ALEN) == 0)
                        return i;       //succeed
        }
        return -1;       //fail
}

int chckmsghdr(struct msghdr *msg_hdr, char *mac_addr, uint16_t ethtyp)
{
        struct sockaddr_ll *rcvaddr;
        rcvaddr = (struct sockaddr_ll*) msg_hdr->msg_name;
        if(rcvaddr->sll_halen != ETH_ALEN)
                return 1;       //fail
        if(rcvaddr->sll_protocol != htons(ethtyp))
                return 1;       //fail
        for(int i=0;i<ETH_ALEN;i++) {
                if(rcvaddr->sll_addr[i] != mac_addr[i])
                        return 1;       //fail
        }
        return 0;
}

int prspkt(struct rt_pkt_t* pkt, enum msgtyp_t *pkttyp)
{
        uint16_t dtstmsgsz = 0;
        int msgfldsz = 0;
        int msgcnt = 0;
        pkt->eth_hdr = (struct eth_hdr_t*)pkt->sktbf;
        pkt->ip_hdr = NULL;
        pkt->udp_hdr = NULL;
        pkt->ntwrkmsg_hdr = /*(struct ntwrkmsg_hdr_t*) pkt->sktbf;*/(struct ntwrkmsg_hdr_t*) ((char *) pkt->eth_hdr + sizeof(struct eth_hdr_t));
        pkt->grp_hdr = (struct grp_hdr_t*) ((char *) pkt->ntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
        pkt->pyld_hdr = (struct pyld_hdr_t*) ((char *) pkt->grp_hdr + sizeof(struct grp_hdr_t));
        msgcnt = pkt->pyld_hdr->msgcnt;
        pkt->extntwrkmsg_hdr = (struct extntwrkmsg_hdr_t*) ((char *) pkt->pyld_hdr + sizeof(pkt->pyld_hdr->msgcnt) + msgcnt*sizeof(pkt->pyld_hdr->wrtrId));
        if (msgcnt < 2){
                //for msgcnt = 1, sizearray is ommitted
                pkt->szrry = NULL;
                msgfldsz = 0;
                if (pkt->len == ETH_ZLEN){
                        dtstmsgsz = sizeof(struct dtstmsg_axs_t);       //axis message is shorter than minimal eth packet length -> padding has been added
                } else {
                        dtstmsgsz = pkt->len - sizeof(struct eth_hdr_t) - sizeof(struct ntwrkmsg_hdr_t) - sizeof(struct grp_hdr_t) - sizeof(struct extntwrkmsg_hdr_t) - sizeof(pkt->pyld_hdr->msgcnt) - msgcnt*sizeof(pkt->pyld_hdr->wrtrId) - msgfldsz;
                }
        } else {
                //for msgcnt >1, set sizes of datasetmessages in sizearray
                pkt->szrry = (struct szrry_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
                msgfldsz = msgcnt*sizeof(*(pkt->szrry));
                dtstmsgsz = ntohs(pkt->szrry->size);
        }
        pkt->dtstmsg = (union dtstmsg_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct extntwrkmsg_hdr_t) + msgfldsz);
        // limitation: only one type of dataset-message in a single packet supported
        switch(dtstmsgsz){
        case sizeof(struct dtstmsg_cntrl_t):
                *pkttyp = CNTRL;
                break;
        case sizeof(struct dtstmsg_axs_t):
                *pkttyp = AXS;
                break;
        default:
                return -1;
        }
              
        return msgcnt;
}

int chckpkthdrs(struct rt_pkt_t* pkt)
{
        //check if flags are the supported paket modes
        //and if group header fields are the same
        if(pkt->ntwrkmsg_hdr->ver_fl != 0xF1)
                return 1;       //fail
        if(pkt->ntwrkmsg_hdr->extfl != 0x21)
                return 1;       //fail
        if(pkt->grp_hdr->wgrpId != htons(WGRPID))
                return 1;       //fail
        if(pkt->grp_hdr->grpVer != htonl(GRPVER))
                return 1;       //fail

        return 0;
}

int prsdtstmsg(struct rt_pkt_t* pkt, enum msgtyp_t pkttyp, union dtstmsg_t *dtstmsgs[], int *dtstmsgcnt)
{
        //limitation: only packets with a single type of dataset-message is supported
        *dtstmsgcnt = 0;
        int dtstmsgsz = 0;
        switch(pkttyp){
        case CNTRL:
                dtstmsgsz = sizeof(struct dtstmsg_cntrl_t);
                break;
        case AXS:
                dtstmsgsz = sizeof(struct dtstmsg_axs_t);
                break;
        default:
                return 1;       //fail
        }
        
        while(*dtstmsgcnt < pkt->pyld_hdr->msgcnt) {
                dtstmsgs[*dtstmsgcnt] = (union dtstmsg_t*) (pkt->dtstmsg + (*dtstmsgcnt)*dtstmsgsz); 

                (*dtstmsgcnt)++;
        }
        return 0;
}

int prsaxsmsg(union dtstmsg_t *dtstmsg, struct axsnfo_t * axsnfo)
{
        //TODO check Datasetflags
        if(dtstmsg->dtstmsg_axs.fldcnt != ntohs(2))    //identifier if truly axis-dataset-message
                return 1;       //fail
        axsnfo->cntrlsw = (bool) dtstmsg->dtstmsg_axs.fault;
        axsnfo->cntrlvl = nint642dbl(be64toh(dtstmsg->dtstmsg_axs.pos_cur));

        return 0;
}

int prscntrlmsg(union dtstmsg_t *dtstmsg, struct cntrlnfo_t * cntrlnfo)
{
        //TODO check Datasetflags
        if(dtstmsg->dtstmsg_axs.fldcnt != ntohs(11))    //identifier if truly axis-dataset-message
                return 1;       //fail
        cntrlnfo->x_set.cntrlvl = nint642dbl(be64toh(dtstmsg->dtstmsg_cntrl.xvel_set));
        cntrlnfo->x_set.cntrlsw = (bool) dtstmsg->dtstmsg_cntrl.xenable;
        cntrlnfo->y_set.cntrlvl = nint642dbl(be64toh(dtstmsg->dtstmsg_cntrl.yvel_set));
        cntrlnfo->y_set.cntrlsw = (bool) dtstmsg->dtstmsg_cntrl.yenable;
        cntrlnfo->z_set.cntrlvl = nint642dbl(be64toh(dtstmsg->dtstmsg_cntrl.zvel_set));
        cntrlnfo->z_set.cntrlsw = (bool) dtstmsg->dtstmsg_cntrl.zenable;
        cntrlnfo->s_set.cntrlvl = nint642dbl(be64toh(dtstmsg->dtstmsg_cntrl.spindlespeed));
        cntrlnfo->s_set.cntrlsw = (bool) dtstmsg->dtstmsg_cntrl.spindleenable;
        cntrlnfo->spindlebrake = (bool) dtstmsg->dtstmsg_cntrl.spindlebrake;
        cntrlnfo->estopstatus = (bool) dtstmsg->dtstmsg_cntrl.estopstatus;
        cntrlnfo->machinestatus = (bool) dtstmsg->dtstmsg_cntrl.machinestatus;

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
        return 0;       //succeded
}

/* ##### END PacketStore ##### */
