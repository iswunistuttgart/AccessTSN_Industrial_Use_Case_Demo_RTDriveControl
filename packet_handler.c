// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "packet_handler.h"
#include <arpa/inet.h>

void Initpkthdrs(struct rt_pkt_t* pkt)
{
        // ntwrkmsg_hdr_t 
        pkt->ntwrkmsg_hdr->ver_fl = 0xF1;       //Version = 1; PublishID, GroupHeader, PayloadHdr and ExtendedFlags 1 enables
        pkt->ntwrkmsg_hdr->extfl = 0x21;        //pubishID datatype Uint16; Timestamp enabled
        //TODO pkt.ntwrkmsg_hdr->pubId;
        // grp_hdr_t
        pkt->grp_hdr->grpfl = 0x0B;             //writergroupID, groupVersion and Sequencenumber enabled
        pkt->grp_hdr->wgrpId = WGRPID;
        pkt->grp_hdr->grpVer =GRPVER;
}

void resetpkt(struct rt_pkt_t* pkt, int msgcnt)
{
        memset(pkt->sktbf,0,MAXPKTSZ*sizeof(char));
        pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyl_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyl_hdr->wrtrId)) + msgcnt*sizeof(struct dtstmsg_t);
        pkt->eth_hdr = NULL;
        pkt->ip_hdr = NULL;
        pkt->udp_hdr = NULL;
        pkt->ntwrkmsg_hdr = pkt->sktbf;
        pkt->grp_hdr = (struct grp_hdr_t*) ((char *) pkt->ntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
        pkt->pyl_hdr = (struct pyl_hdr_t*) ((char *) pkt->grp_hdr + sizeof(struct grp_hdr_t));
        pkt->extntwrkmsg_hdr = (struct extntwrkmsg_hdr_t*) ((char *) pkt->pyl_hdr + sizeof(pkt->pyl_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyl_hdr->wrtrId)));
        pkt->dtstmsg = (struct dtstmsg_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct extntwrkmsg_hdr_t));
}

int createpkt(struct rt_pkt_t* pkt, int msgcnt)
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

        resetpkt(newpkt,msgcnt);

        pkt = newpkt;
        return 0;       //succeded
}

void destroypkt(struct rt_pkt_t*pkt)
{
        pkt->eth_hdr = NULL;
        pkt->ip_hdr = NULL;
        pkt->udp_hdr = NULL;
        pkt->ntwrkmsg_hdr = NULL;
        pkt->grp_hdr = NULL;
        pkt->pyl_hdr = NULL;
        pkt->extntwrkmsg_hdr = NULL;
        pkt->dtstmsg = NULL;
        free(pkt->sktbf);
        pkt->len = 0;
        pkt->sktbf = NULL;
        free(pkt);
        pkt = NULL;
}