// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/* Demoapplication to send values taken from shared memory to a drive via TSN
 */

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include <linux/net_tstamp.h>
#include "packet_handler.h"
#include "axisshm_handler.h"


//parameters whcih are fixed at compile time, all values in nano seconds
#define SENDINGSTACK_DURATION 100000 
#define RECEIVINGSTACK_DURATION 100000
#define APPSENDWAKEUP 100000
#define APPRECVWAKEUP 100000
#define MAXWAKEUPJITTER 40000

uint8_t run = 1;

struct cnfg_optns_t{
        struct timespec basetm;
        uint32_t intrvl_ns;
        uint32_t sndoffst;
        uint32_t rcvoffst;
        uint32_t rcvwndw;
        char * dstaddr;
        char * ifname;
        char *rcv_macs[4];
        uint8_t num_rcvmacs;
        uint16_t pubid;
        int prrty;
};

struct tsnsender_t {
        struct cnfg_optns_t cnfg_optns;
        int rxsckt;
        int txsckt;
        struct mk_mainoutput *txshm;
        struct mk_maininput *rxshm;
        struct mk_additionaloutput *atxshm;
        sem_t* txshm_sem;
        sem_t* rxshm_sem;
        sem_t* atxshm_sem;
        struct pktstore_t pkts;
        pthread_attr_t rtthrd_attr;
        pthread_t rt_thrd;
        pthread_attr_t rxthrd_attr;
        pthread_t rx_thrd;
};

/* signal handler */
void sigfunc(int sig)
{
        switch(sig)
        {
        case SIGINT:
                if(run)
                        run = 0;
                else
                        exit(0);
                break;
        case SIGTERM:
                run = 0;
                break;        
        }
}

/* Print usage message */
static void usage(char *appname)
{
        fprintf(stderr,
                "\n"
                "Usage: %s [options]\n"
                " -t [value]           Specifies update-period in microseconds. Default 1 millisecond.\n"
                " -b [value]           Specifies the basetime (the start of the cycle). This is a Unix-Timestamp.\n"
                " -o [nanosec]         Specifies the sending offset, time between start of cycle and sending slot in nano seconds.\n"
                " -r [nanosec]         Specifies the receiving offset, time between start of cycle and end of receive slot in nano seconds.\n"
                " -w [nanosec]         Specifies the receive window duration, timeinterval in which a packet is expected in nano seconds.\n"
                " -i                   Name of the Networkinterface to use.\n"
                " -p                   PublisherID e.g. TalkerID, Default: 0xAC00\n"
                " -y                   Priority of sending socket (can be 1-7), Default: 6\n"
                " -h                   Prints this help message and exits\n"
                "\n",
                appname);
}

/* Evaluate CLI-parameters */
void evalCLI(int argc, char* argv[0],struct tsnsender_t * sender)
{
        int c;
        //set standard values
        cnvrt_dbl2tmspc(0, &(sender->cnfg_optns.basetm));
        sender->cnfg_optns.intrvl_ns = 1000000;
        sender->cnfg_optns.pubid = 0xAC00;
        sender->cnfg_optns.prrty = 6;

        char* appname = strrchr(argv[0], '/');
        appname = appname ? 1 + appname : argv[0];
        while (EOF != (c = getopt(argc,argv,"ht:b:o:r:w:i:p:y:"))) {
                switch(c) {
                case 'b':
                        cnvrt_dbl2tmspc(atof(optarg), &(sender->cnfg_optns.basetm));
                        break;
                case 't':
                        sender->cnfg_optns.intrvl_ns = atoi(optarg)*1000;
                        break;
                case 'o':
                        sender->cnfg_optns.sndoffst = atoi(optarg);
                        break;
                case 'r':
                        sender->cnfg_optns.rcvoffst = atoi(optarg);
                        break;
                case 'w':
                        sender->cnfg_optns.rcvwndw = atoi(optarg);
                        break;
                case 'i':
                        sender->cnfg_optns.ifname = calloc(strlen(optarg),sizeof(char));
                        strcpy(sender->cnfg_optns.ifname,optarg);
                        break;
                case 'p':
                        sender->cnfg_optns.pubid = atoi(optarg);
                        break;
                case 'y':
                        sender->cnfg_optns.prrty = atoi(optarg);
                        break;
                case 'h':
                default:
                        usage(appname);
                        exit(0);
                        break;
                }
        }
        if ((sender->cnfg_optns.prrty < 1) || (sender->cnfg_optns.prrty > 7)) {
                printf("Specified socket priority is out of rage. Must be between 1 and 7.\n");
                exit(0);
        }
}

// open tx socket
int opntxsckt(int prrty)
{       
        int ok;
        struct sock_txtime soctxtm;
        int sckt = socket(AF_PACKET,SOCK_DGRAM,ETHERTYPE);
        if (sckt < 0)
                return sckt;      //fail
        soctxtm.clockid = CLOCK_TAI;
        soctxtm.flags = 0;
        ok = setsockopt(sckt,SOL_SOCKET,SO_TXTIME,&soctxtm,sizeof(soctxtm));
        if (ok != 0)
                printf("Warning: Setting of Socketoption TXTIME failed (TX). Error: %d \n",errno);
        int prio = prrty;
        ok = setsockopt(sckt, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(prio));
        if (ok != 0)
                printf("Warning: Setting of socket priority failed (TX). Error: %d \n",errno);
        
        return sckt;
}

//initialization
int init(struct tsnsender_t *sender)
{
        int ok = 0;
        struct sched_param param;

        //open send socket
        sender->txsckt = opntxsckt(sender->cnfg_optns.prrty);
        if (sender->txsckt < 0) {
                printf("TX Socket open failed. \n");
                sender->txsckt = 0;
                return 1;
        }

        //open recv socket
        sender->rxsckt = opnrxsckt(sender->cnfg_optns.ifname,sender->cnfg_optns.rcv_macs,sender->cnfg_optns.num_rcvmacs);
        if (sender->rxsckt < 0) {
                printf("RX Socket open failed. \n");
                sender->rxsckt = 0;
                return 1;
        }

        //open shared memory
        sender->txshm = opnShM_cntrlnfo(&sender->txshm_sem);
        if(sender->txshm == NULL) {
                printf("open of TX sharedmemory failed\n");
                return 1;
        };
        sender->atxshm = opnShM_addcntrlnfo(&sender->atxshm_sem);
        if(sender->atxshm == NULL) {
                printf("open of additional TX sharedmemory failed\n");
                return 1;
        };
        sender->rxshm = opnShM_axsnfo(&sender->rxshm_sem);
        if(sender->txshm == NULL) {
                printf("open of RX sharedmemory failed\n");
                return 1;
        };

        //allocate memory for packets
        ok += initpktstrg(&(sender->pkts),5);

        //prefault stack/heap --> done by mlocking APIs

        // ### setup rt_thread
        //Lock memory --> maybe only lock necessary pages (not all) using mlock
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                return -2;
        }
        //Initialize pthread attributes (default values)
        ok = pthread_attr_init(&(sender->rtthrd_attr));
        if (ok) {
                printf("init pthread attributes failed\n");
                return 1; //fail
        }
        //Set a specific stack size 
        ok = pthread_attr_setstacksize(&(sender->rtthrd_attr), PTHREAD_STACK_MIN);
        if (ok) {
            printf("pthread setstacksize failed\n");
            return 1;   //fail
        }
 
        //Set scheduler policy and priority of pthread
        ok = pthread_attr_setschedpolicy(&(sender->rtthrd_attr), SCHED_FIFO);
        if (ok) {
                printf("pthread setschedpolicy failed\n");
                return 1;
        }
        pthread_attr_getschedparam(&(sender->rtthrd_attr),&param);
        param.sched_priority = 80;
        ok = pthread_attr_setschedparam(&(sender->rtthrd_attr), &param);
        if (ok) {
                printf("pthread setschedparam failed\n");
                return 1;
        }
        //Use scheduling parameters of attr
        ok = pthread_attr_setinheritsched(&(sender->rtthrd_attr), PTHREAD_EXPLICIT_SCHED);
        if (ok) {
                printf("pthread setinheritsched failed\n");
                return 1;
        }
        //detach thread since returnvalue does not matter
        ok = pthread_attr_setdetachstate(&(sender->rtthrd_attr),PTHREAD_CREATE_DETACHED);
        if (ok) {
                printf("pthread setdetached failed\n");
                return 1;
        }

        // ##### setup rx-thread
        //Initialize pthread attributes (default values)
        ok = pthread_attr_init(&(sender->rxthrd_attr));
        if (ok) {
                printf("init pthread attributes failed\n");
                return 1; //fail
        }
        //Set a specific stack size 
        ok = pthread_attr_setstacksize(&(sender->rxthrd_attr), PTHREAD_STACK_MIN);
        if (ok) {
            printf("pthread setstacksize failed\n");
            return 1;   //fail
        }
 
        //Set scheduler policy and priority of pthread
        ok = pthread_attr_setschedpolicy(&(sender->rxthrd_attr), SCHED_FIFO);
        if (ok) {
                printf("pthread setschedpolicy failed\n");
                return 1;
        }
        pthread_attr_getschedparam(&(sender->rxthrd_attr),&param);
        param.sched_priority = 75;
        ok = pthread_attr_setschedparam(&(sender->rxthrd_attr), &param);
        if (ok) {
                printf("pthread setschedparam failed\n");
                return 1;
        }
        //Use scheduling parameters of attr
        ok = pthread_attr_setinheritsched(&(sender->rxthrd_attr), PTHREAD_EXPLICIT_SCHED);
        if (ok) {
                printf("pthread setinheritsched failed\n");
                return 1;
        }
        //detach thread since returnvalue does not matter
        ok = pthread_attr_setdetachstate(&(sender->rxthrd_attr),PTHREAD_CREATE_DETACHED);
        if (ok) {
                printf("pthread setdetached failed\n");
                return 1;
        }

        //OPTIONAL: setup pmc-thread (optional)

        return ok;
}

//End execution with cleanup
int cleanup(struct tsnsender_t *sender)
{
        int ok = 0;
        //stop threads
        ok = pthread_cancel(sender->rt_thrd);
        ok =+ pthread_cancel(sender->rx_thrd);

        //close rx socket
        ok += close(sender->rxsckt);
        
        //close tx socket
        ok += close(sender->txsckt);

        //close shared memory
        ok += clscntrlShM(&(sender->txshm),&sender->txshm_sem);
        ok += clsaddcntrlShM(&(sender->atxshm),&sender->atxshm_sem);
        ok += clsaxsShM(&(sender->rxshm),&sender->rxshm_sem);

        //free allocated memory for packets
        ok += destroypktstrg(&(sender->pkts));

        free(sender->cnfg_optns.dstaddr);
        for (int i = 0;i<4;i++){
                if (sender->cnfg_optns.rcv_macs[i] != NULL) {
                        free(sender->cnfg_optns.rcv_macs[i]);
                        sender->cnfg_optns.rcv_macs[i] =NULL;
                }
        }
        return ok;
}

//Real time thread sender
void *rt_thrd(void *tsnsender)
{
	int ok;
        struct tsnsender_t *sender = (struct tsnsender_t *) tsnsender;
        struct timespec est;
        struct timespec wkupsndtm;
        struct timespec txtime;
        struct rt_pkt_t *snd_pkt;
        struct sockaddr_ll snd_addr;
        struct cntrlnfo_t snd_cntrlnfo;
        memset(&snd_cntrlnfo,0, sizeof(struct cntrlnfo_t));
        uint16_t snd_seqno = 0;
        
        struct timespec cntrlrd_tmout;

        //init sending address since it will be static
        ok = fillethaddr(&snd_addr, sender->cnfg_optns.dstaddr, ETHERTYPE, sender->txsckt, sender->cnfg_optns.ifname);
	                
        /*sleep this (basetime minus one period) is reached
        or (basetime plus multiple periods) */
        clock_gettime(CLOCK_TAI,&wkupsndtm);
        clc_est(&wkupsndtm,&(sender->cnfg_optns.basetm), sender->cnfg_optns.intrvl_ns, &est);      //check if added period is enough time buffer, maybe increase to two
        txtime = clc_txtm(&est,sender->cnfg_optns.sndoffst,SENDINGSTACK_DURATION);
        wkupsndtm = clc_sndwkuptm(&txtime,APPSENDWAKEUP,MAXWAKEUPJITTER);
        tmspc_cp(&cntrlrd_tmout,&wkupsndtm);
        inc_tm(&cntrlrd_tmout,APPSENDWAKEUP/2);

        //sleep till first wakeup time
        clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &wkupsndtm, NULL);
        
	
        //while loop
        while(true){
                //get TX values from shared memory
                ok = rd_shm2cntrlinfo(sender->txshm, &snd_cntrlnfo, sender->txshm_sem, &cntrlrd_tmout);

                //get and fill TX-Packet
                ok = getfreepkt(&(sender->pkts),&snd_pkt);       //maybe change to one static packet in thread to avoid competing access to paket store from rx and tx threads
                if (ok == 1){
                        printf("Could not get free packet for sending. \n");
                        return NULL;       //fail
                }
                ok += setpkt(snd_pkt,1,CNTRL,sender->cnfg_optns.pubid);
                
                ok += fillcntrlpkt(snd_pkt,&snd_cntrlnfo,snd_seqno);
                if (ok != 0){
                        printf("Error in filling sending packet or corresponding headers.\n");
                        return NULL;       //fail
                }
                //send TX-Packet
                ok += sendpkt(sender->txsckt,snd_pkt->sktbf,snd_pkt->len,&snd_addr,cnvrt_tmspc2int64(&txtime),CLOCK_TAI);
                if (0 == ok)
                        snd_seqno++;    //sending packet succeded

                //return packet to store
                ok += retusedpkt(&(sender->pkts),&snd_pkt);

                //update time
                inc_tm(&est,sender->cnfg_optns.intrvl_ns);
                //calculate next TxTime-Stamp and next wakeuptime
                txtime = clc_txtm(&est,sender->cnfg_optns.sndoffst,SENDINGSTACK_DURATION);
                wkupsndtm = clc_sndwkuptm(&txtime,APPSENDWAKEUP,MAXWAKEUPJITTER);
                tmspc_cp(&cntrlrd_tmout,&wkupsndtm);
                inc_tm(&cntrlrd_tmout,APPSENDWAKEUP/2);
                //sleep until the next cycle
                clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &wkupsndtm, NULL);
        }
        return NULL;
}

//Real time recv thread
void *rx_thrd(void *tsnsender)
{
	int ok = 0;
        int rcv_cnt = 1;
        struct tsnsender_t *sender = (struct tsnsender_t *) tsnsender;
        struct timespec est;
        struct timespec wkuprcvtm;
        struct timespec curtm;
        
        struct pollfd fds[1] = {};
        fds[0].fd = sender->rxsckt;
        fds[0].events = POLLIN;
        int tmout;
        if (sender->cnfg_optns.rcvwndw >= 1000000) {
                tmout = sender->cnfg_optns.rcvwndw/1000000;
        } else {
                tmout = 0;
        }

	struct rt_pkt_t * rcvd_pkt;
        struct msghdr rcvd_msghdr;
        enum msgtyp_t msg_typ;
        union dtstmsg_t *dtstmsgs[4] = {NULL,NULL,NULL,NULL};
        int dtstmsgcnt;
        struct axsnfo_t axs_nfo;
        struct timespec axswrt_tmout;
        uint32_t axswrt_tmoutfrac;
        axswrt_tmoutfrac = sender->cnfg_optns.intrvl_ns/(sender->cnfg_optns.num_rcvmacs+1);
                
        /*sleep this (basetime minus one period) is reached
        or (basetime plus multiple periods), calculated fitting offset to recv */
        clock_gettime(CLOCK_TAI,&wkuprcvtm);
        clc_est(&wkuprcvtm,&(sender->cnfg_optns.basetm), sender->cnfg_optns.intrvl_ns, &est);
        wkuprcvtm = clc_rcvwkuptm(&est,sender->cnfg_optns.rcvoffst,RECEIVINGSTACK_DURATION,APPRECVWAKEUP,MAXWAKEUPJITTER);
        tmspc_cp(&axswrt_tmout,&wkuprcvtm);
        inc_tm(&axswrt_tmout,axswrt_tmoutfrac);
                
        //while loop
        while(true){
                
                //for more than one axis, multiple packets should arrive within a period
                if((rcv_cnt%(sender->cnfg_optns.num_rcvmacs+1)) == 0){
                        // nanosleep at start of cycle because of "continue" statement
                        //update time
                        /* can be done more simple when recv_offset cannot change during operation
                        inc_tm(&est,sender->cnfg_optns.intrvl_ns);
                        //calculate next wakeuptime
                        wkuprcvtm = clc_rcvwkuptm(&est,sender->cnfg_optns.rcvoffst,RECEIVINGSTACK_DURATION,APPRECVWAKEUP,MAXWAKEUPJITTER);
                        */
                        //more simple
                        clock_gettime(CLOCK_TAI,&curtm);
                        while(cmptmspc_Ab4rB(&wkuprcvtm,&curtm)){
                                //if no (valid) packet arrives the receive could wait longer than a period
                                inc_tm(&wkuprcvtm,sender->cnfg_optns.intrvl_ns);
                        }
                        tmspc_cp(&axswrt_tmout,&wkuprcvtm);
                        inc_tm(&axswrt_tmout,axswrt_tmoutfrac);
                        //sleep until the next cycle
                        clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &wkuprcvtm, NULL);
                        rcv_cnt = 1;
                }

                //check for RX-packet
                ok = poll(fds,1,tmout);
                if (ok <= 0)
                        continue;
               
                //receive paket
                ok = getfreepkt(&(sender->pkts),&rcvd_pkt);
                if (ok == 1) {
                        printf("Could not get free packet for receiving. \n");
                        return NULL;       //fail
                }
                ok = rcvpkt(sender->rxsckt, rcvd_pkt, &rcvd_msghdr);
                if (ok == 1) {
                        printf("Receive failed. \n");
                        retusedpkt(&(sender->pkts),&rcvd_pkt);
                        return NULL;       //fail
                }

                // check ETH-header
                axs_nfo.axsID = chckethhdr(rcvd_pkt, sender->cnfg_optns.rcv_macs, sender->cnfg_optns.num_rcvmacs);
                if (axs_nfo.axsID == -1) {
                        printf("Check ETH-Header failed. \n");
                        retusedpkt(&(sender->pkts),&rcvd_pkt);
                        continue;
                }
                //parse RX-packet
                ok = prspkt(rcvd_pkt, &msg_typ);
                if ((ok == -1) || (msg_typ != AXS)) {
                        printf("Parsing of received packet failed, or packet not a AXS-packet. typ %d; ok: %d\n",msg_typ, ok);
                        retusedpkt(&(sender->pkts),&rcvd_pkt);
                        continue;
                }
                ok = chckpkthdrs(rcvd_pkt);
                if (ok == 1) {
                        printf("Check Packet-Headers failed. \n");
                        retusedpkt(&(sender->pkts),&rcvd_pkt);
                        continue;
                }
                ok = prsdtstmsg(rcvd_pkt, msg_typ, dtstmsgs, &dtstmsgcnt);
                
                //write RX values to shared memory
                for (int i = 0;i<dtstmsgcnt; i++) {
                        ok = prsaxsmsg(dtstmsgs[i],&axs_nfo);
                        if (dtstmsgcnt > 1)  {
                                //only one datasetmsg in packet, axs differentation through rcvmac
                                //could also be done through different writer ids
                                axs_nfo.axsID = i;
                        }
                        printf("write axisinfo to shm for axis %d, value %f\n",axs_nfo.axsID,axs_nfo.cntrlvl);
                        ok =  wrt_axsinfo2shm(&axs_nfo, sender->rxshm,sender->rxshm_sem,&axswrt_tmout);
                        inc_tm(&axswrt_tmout,axswrt_tmoutfrac);
                        dtstmsgs[i] = NULL;
                }

                retusedpkt(&(sender->pkts),&rcvd_pkt);
                rcv_cnt++;
        }

        return NULL;
}


int main(int argc, char* argv[])
{
        struct tsnsender_t sender;
        int ok;
        unsigned char mac[ETH_ALEN];

        //set standard values for addresses
        sender.cnfg_optns.dstaddr = calloc(ETH_ALEN,sizeof(char));
        sscanf(DSTADDRCNTRL,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(sender.cnfg_optns.dstaddr,mac,ETH_ALEN);

        sender.cnfg_optns.rcv_macs[0] = calloc(ETH_ALEN,sizeof(char));
        sender.cnfg_optns.rcv_macs[1] = calloc(ETH_ALEN,sizeof(char));
        sender.cnfg_optns.rcv_macs[2] = calloc(ETH_ALEN,sizeof(char));
        sender.cnfg_optns.rcv_macs[3] = calloc(ETH_ALEN,sizeof(char));

        sscanf(DSTADDRAXSX,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(sender.cnfg_optns.rcv_macs[0],mac,ETH_ALEN);
        sscanf(DSTADDRAXSY,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(sender.cnfg_optns.rcv_macs[1],mac,ETH_ALEN);
        sscanf(DSTADDRAXSZ,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(sender.cnfg_optns.rcv_macs[2],mac,ETH_ALEN);
        sscanf(DSTADDRAXSS,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(sender.cnfg_optns.rcv_macs[3],mac,ETH_ALEN);
        sender.cnfg_optns.num_rcvmacs = 4;
      
        //parse CLI arguments
        evalCLI(argc,argv,&sender);

        //init (including real-time)
        ok = init(&sender);
        if(ok != 0){
                printf("Initialization failed\n");
                //cleanup
                cleanup(&sender);
                return ok;       //fail
        }
        
        //register signal handlers
        signal(SIGTERM, sigfunc);
        signal(SIGINT, sigfunc);

        //start rt-thread   
        /* Create a pthread with specified attributes */
        ok = pthread_create(&(sender.rt_thrd), &(sender.rtthrd_attr), (void*) rt_thrd, (void*)&sender);
        ok += pthread_create(&(sender.rx_thrd),&(sender.rxthrd_attr),(void*) rx_thrd, (void*) &sender);
        if (ok) {
                printf("create pthread failed\n");
                //cleanup
                cleanup(&sender);
                return 1;      //fail
        }
 
        /* Join the thread and wait until it is done */
	int ret;
        ret = pthread_join((sender.rx_thrd), NULL);
        if (ret)
                printf("join pthread failed: %m\n");
        

        //OPTIONAL: start pmc-thread

        while(run){
                sleep(1);
        }

        // cleanup
        ok = cleanup(&sender);

        return 0;       //succeded
}
