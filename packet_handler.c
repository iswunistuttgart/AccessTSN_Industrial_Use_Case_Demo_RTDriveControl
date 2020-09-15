// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "packet_handler.h"
#include <arpa/inet.h>

void initpkthdrs(struct rt_pkt_t* pkt)
{
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
        memset(pkt->sktbf,0,MAXPKTSZ*sizeof(char));
        pkt->eth_hdr = NULL;
        pkt->ip_hdr = NULL;
        pkt->udp_hdr = NULL;
        pkt->ntwrkmsg_hdr = pkt->sktbf;
        pkt->grp_hdr = (struct grp_hdr_t*) ((char *) pkt->ntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
        pkt->pyl_hdr = (struct pyl_hdr_t*) ((char *) pkt->grp_hdr + sizeof(struct grp_hdr_t));
        pkt->pyl_hdr->msgcnt = msgcnt;
        pkt->extntwrkmsg_hdr = (struct extntwrkmsg_hdr_t*) ((char *) pkt->pyl_hdr + sizeof(pkt->pyl_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyl_hdr->wrtrId)));
        pkt->szrry = (struct szrry_t*) ((char *) pkt->extntwrkmsg_hdr + sizeof(struct ntwrkmsg_hdr_t));
        
        switch(msgtyp){
        case CNTRL:
                pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyl_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyl_hdr->wrtrId)) + (msgcnt - 1)*sizeof(*(pkt->szrry)) + msgcnt*sizeof(struct dtstmsg_cntrl_t);
                pkt->dtstmsg->dtstmsg_cntrl = (struct dtstmsg_cntrl_t*) ((char *) pkt->szrry + (msgcnt - 1)*sizeof(*(pkt->szrry)));
                dtstmsgsz = sizeof(struct dtstmsg_cntrl_t);
                pkt->dtstmsg->dtstmsg_cntrl->dtstmsg_hdr = 0x01;
                pkt->dtstmsg->dtstmsg_cntrl->fldcnt = 11;
                break;
        case AXS:
                pkt->len = sizeof(struct ntwrkmsg_hdr_t) + sizeof(struct grp_hdr_t) + sizeof(struct extntwrkmsg_hdr_t) + sizeof(pkt->pyl_hdr->msgcnt) + msgcnt*sizeof(*(pkt->pyl_hdr->wrtrId)) + (msgcnt - 1)*sizeof(*(pkt->szrry)) + msgcnt*sizeof(struct dtstmsg_axs_t);
                pkt->dtstmsg->dtstmsg_axs = (struct dtstmsg_axs_t*) ((char *) pkt->szrry + (msgcnt - 1)*sizeof(*(pkt->szrry)));
                dtstmsgsz = sizeof(struct dtstmsg_axs_t);
                pkt->dtstmsg->dtstmsg_axs->dtstmsg_hdr = 0x01;
                pkt->dtstmsg->dtstmsg_axs->fldcnt = 2;
                break;
        default:
                return 1;       //fail
        };
        int i = 0;
        if (msgcnt < 2){
                //for msgcnt = 1, sizearray is ommitted
                pkt->szrry = NULL;
        } else {
                //for msgcnt >1, set sizes of datasetmessages in sizearray
                while (i <msgcnt) {
                        *(pkt->szrry->size + i) = dtstmsgsz;
                        i++;
                }
        }
        
        initpkthdrs(pkt);
        
        return 0;       //succeded
}

int createpkt(struct rt_pkt_t* pkt)
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

int fillcntrlpkt(struct rt_pkt_t* pkt, struct cntrlnfo_t* cntrlnfo)
{
        int64_t tmp;
        int ok = 0;
        pkt->pyl_hdr->wrtrId = WRITERID_CNTRL;
        ok += dbl2nint64(cntrlnfo->x_set.cntrlvl,&tmp);
        pkt->dtstmsg->dtstmsg_cntrl->xvel_set = htobe64(tmp);
        pkt->dtstmsg->dtstmsg_cntrl->xenable = (uint8_t) cntrlnfo->x_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->y_set.cntrlvl,&tmp);
        pkt->dtstmsg->dtstmsg_cntrl->yvel_set = htobe64(tmp);
        pkt->dtstmsg->dtstmsg_cntrl->yenable = (uint8_t) cntrlnfo->y_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->z_set.cntrlvl,&tmp);
        pkt->dtstmsg->dtstmsg_cntrl->zvel_set = htobe64(tmp);
        pkt->dtstmsg->dtstmsg_cntrl->zenable = (uint8_t) cntrlnfo->z_set.cntrlsw;
        ok += dbl2nint64(cntrlnfo->s_set.cntrlvl,&tmp);
        pkt->dtstmsg->dtstmsg_cntrl->spindlespeed = htobe64(tmp);
        pkt->dtstmsg->dtstmsg_cntrl->spindleenable = (uint8_t) cntrlnfo->s_set.cntrlsw;
        pkt->dtstmsg->dtstmsg_cntrl->spindlebrake = (uint8_t) cntrlnfo->spindlebrake;
        pkt->dtstmsg->dtstmsg_cntrl->machinestatus = (uint8_t) cntrlnfo->machinestatus;
        pkt->dtstmsg->dtstmsg_cntrl->estopstatus = (uint8_t) cntrlnfo->estopstatus;

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