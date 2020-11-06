// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/* Demoapplication to receive values via TSN and simulate a drive
 * 
 * Usage:
 * -d [IP-address]      Destination IP-Address (use dot-notation)
 * -t [value]           Specifies update-period in milliseconds. Default 10 seconds
 * -h                   Prints this help message and exits
 * 
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
#include "packet_handler.h"
#include "axis_sim.h"

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
        uint32_t sndwndw;
        uint32_t rcvoffst;
        uint32_t rcvwndw;
        char * rcvaddr[1];
        char * ifname;
        char * snd_macs[4];
        uint8_t num_axs;
        enum axsID_t frst_axs;
};

struct tsndrive_t {
        struct cnfg_optns_t cnfg_optns;
        int rxsckt;
        int txsckt;
        struct pktstore_t pkts;
        struct axis_t * axes[4];
        pthread_attr_t rtthrd_attr;
        pthread_t rt_thrd;
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
                " -t [value]           Specifies update-period in microseconds. Default 10 seconds.\n"
                " -b [value]           Specifies the basetime (the start of the cycle). This is a Unix-Timestamp.\n"
                " -o [nanosec]         Specifies the sending offset, time between start of cycle and sending slot in nano seconds.\n"
                " -r [nanosec]         Specifies the receiving offset, time between start of cycle and end of receive slot in nano seconds.\n"
                " -w [nanosec]         Specifies the receive window duration, timeinterval in which a packet is expected in nano seconds.\n"
                " -s [nanosec]         Specifies the send window duration, timeinterval between 2 axis-packets in nano seconds.\n"
                " -i [name]            Name of the Networkinterface to use.\n"
                " -n [value < 5]       Number of simulated axes. Default 4.\n"
                " -a [index < 4]       Index of the first simulated axis. x = 0, y = 1, z = 2, spindle = 3. Default 0.\n"
                " -h                   Prints this help message and exits\n"
                "\n",
                appname);
}

/* Evaluate CLI-parameters */
void evalCLI(int argc, char* argv[0],struct tsndrive_t * drivesim)
{
        int c;
        char* appname = strrchr(argv[0], '/');
        appname = appname ? 1 + appname : argv[0];
        while (EOF != (c = getopt(argc,argv,"ht:b:o:r:w:s:i:n:a:"))) {
                switch(c) {
                case 'b':
                        cnvrt_dbl2tmspc(atof(optarg), &(drivesim->cnfg_optns.basetm));
                        break;
                case 't':
                        (*drivesim).cnfg_optns.intrvl_ns = atoi(optarg)*1000;
                        break;
                case 'o':
                        (*drivesim).cnfg_optns.sndoffst = atoi(optarg);
                        break;
                case 'r':
                        (*drivesim).cnfg_optns.rcvoffst = atoi(optarg);
                        break;
                case 'w':
                        (*drivesim).cnfg_optns.rcvwndw = atoi(optarg);
                        break;
                case 's':
                        (*drivesim).cnfg_optns.sndwndw = atoi(optarg);
                        break;
                case 'i':
                        (*drivesim).cnfg_optns.ifname = calloc(strlen(optarg),sizeof(char));
                        strcpy((*drivesim).cnfg_optns.ifname,optarg);
                        break;
                case 'n':
                        (*drivesim).cnfg_optns.num_axs = atoi(optarg);
                        break;
                case 'a':
                        (*drivesim).cnfg_optns.frst_axs = atoi(optarg);
                        break;
                case 'h':
                default:
                        usage(appname);
                        exit(0);
                        break;
                }
        }
        if ((*drivesim).cnfg_optns.num_axs > 4) {
                printf("Number of simulated Axis to high! Maximum 4.\n");
                exit(0);
        }
        if ((*drivesim).cnfg_optns.frst_axs > 3) {
                printf("Axis index to high! Maximum 4.\n");
                exit(0);
        }
        if (((*drivesim).cnfg_optns.frst_axs + (*drivesim).cnfg_optns.num_axs) > 4) {
                printf("Combination of Axis index and number of simulation Axis to high!\n");
                exit(0);   
        }
}

// open tx socket
int opntxsckt(void)
{
        int sckt = socket(AF_PACKET,SOCK_DGRAM,ETHERTYPE);
        if (sckt < 0)
                return sckt;      //fail
        const int on = 1;
        //setsockopt(sckt,SOL_SOCKET,SO_TXTIME,&on,sizeof(on));
        //maybe need to set SO_BROADCAST also
        return sckt;
}

//initialization
int init(struct tsndrive_t *drivesim)
{
        int ok = 0;
        struct sched_param param;
        unsigned char mac[ETH_ALEN];

        //set addresses
        //set standard values
        drivesim->cnfg_optns.rcvaddr[0] = calloc(ETH_ALEN,sizeof(char));
        sscanf(DSTADDRCNTRL,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        strncpy(drivesim->cnfg_optns.rcvaddr[0],mac,ETH_ALEN);

        drivesim->cnfg_optns.snd_macs[0] = calloc(ETH_ALEN,sizeof(char));
        drivesim->cnfg_optns.snd_macs[1] = calloc(ETH_ALEN,sizeof(char));
        drivesim->cnfg_optns.snd_macs[2] = calloc(ETH_ALEN,sizeof(char));
        drivesim->cnfg_optns.snd_macs[3] = calloc(ETH_ALEN,sizeof(char));

        sscanf(DSTADDRAXSX,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        strncpy(drivesim->cnfg_optns.snd_macs[0],mac,ETH_ALEN);
        sscanf(DSTADDRAXSY,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        strncpy(drivesim->cnfg_optns.snd_macs[1],mac,ETH_ALEN);
        sscanf(DSTADDRAXSZ,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        strncpy(drivesim->cnfg_optns.snd_macs[2],mac,ETH_ALEN);
        sscanf(DSTADDRAXSS,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        strncpy(drivesim->cnfg_optns.snd_macs[3],mac,ETH_ALEN);
        
        //open send socket
        drivesim->txsckt = opntxsckt();
        if (drivesim->txsckt < 0) {
                printf("TX Socket open failed. \n");
                drivesim->txsckt = 0;
                return 1;
        }

        //open recv socket
        drivesim->rxsckt = opnrxsckt(drivesim->cnfg_optns.ifname,drivesim->cnfg_optns.snd_macs,drivesim->cnfg_optns.num_axs);
        if (drivesim->rxsckt < 0) {
                printf("RX Socket open failed. \n");
                drivesim->rxsckt = 0;
                return 1;
        }
        
        //allocate memory for packets
        ok += initpktstrg(&(drivesim->pkts),6);

        //create correct no of axis
        for  (int i = 0; i < drivesim->cnfg_optns.num_axs;i++) {
                drivesim->axes[i] = calloc(1,sizeof(struct axis_t));
                if (NULL == drivesim->axes[i])
                return 1;       //fail
        }
        for  (int i = drivesim->cnfg_optns.num_axs; i < 4;i++) {
                drivesim->axes[i] = NULL;
        }
        ok = axes_initreq(drivesim->axes, drivesim->cnfg_optns.num_axs, drivesim->cnfg_optns.frst_axs);

        //prefault stack/heap --> done by mlocking APIs

        // ### setup rt_thread
        //Lock memory --> maybe only lock necessary pages (not all) using mlock
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                return -2;
        }
        //Initialize pthread attributes (default values)
        ok = pthread_attr_init(&(drivesim->rtthrd_attr));
        if (ok) {
                printf("init pthread attributes failed\n");
                return 1; //fail
        }
        //Set a specific stack size 
        ok = pthread_attr_setstacksize(&(drivesim->rtthrd_attr), PTHREAD_STACK_MIN);
        if (ok) {
            printf("pthread setstacksize failed\n");
            return 1;   //fail
        }
 
        //Set scheduler policy and priority of pthread
        ok = pthread_attr_setschedpolicy(&(drivesim->rtthrd_attr), SCHED_FIFO);
        if (ok) {
                printf("pthread setschedpolicy failed\n");
                return 1;
        }
        pthread_attr_getschedparam(&(drivesim->rtthrd_attr),&param);
        param.sched_priority = 80;
        ok = pthread_attr_setschedparam(&(drivesim->rtthrd_attr), &param);
        if (ok) {
                printf("pthread setschedparam failed\n");
                return 1;
        }
        //Use scheduling parameters of attr
        ok = pthread_attr_setinheritsched(&(drivesim->rtthrd_attr), PTHREAD_EXPLICIT_SCHED);
        if (ok) {
                printf("pthread setinheritsched failed\n");
                return 1;
        }
        //detach thread since returnvalue does not matter
        ok = pthread_attr_setdetachstate(&(drivesim->rtthrd_attr),PTHREAD_CREATE_DETACHED);
        if (ok) {
                printf("pthread setdetached failed\n");
                return 1;
        }

        //setup pmc-thread (optional)

        return ok;
}

//End execution with cleanup
int cleanup(struct tsndrive_t *drivesim)
{
        int ok = 0;
        //stop threads
        ok = pthread_cancel(drivesim->rt_thrd);
        //maybe need to wait until thread has ended?

        //close rx socket
        ok += close(drivesim->rxsckt);
        
        //close tx socket
        ok += close(drivesim->txsckt);

        //free allocated memory for packets
        ok += destroypktstrg(&(drivesim->pkts));

        free(drivesim->cnfg_optns.rcvaddr[0]);
        for (int i = 0;i<4;i++){
                if (drivesim->cnfg_optns.snd_macs[i]!= NULL) {
                        free(drivesim->cnfg_optns.snd_macs[i]);
                        drivesim->cnfg_optns.snd_macs[i] = NULL;
                }
                if (drivesim->axes[i]!= NULL) {
                        free(drivesim->axes[i]);
                        drivesim->axes[i] = NULL;
                }
        }
        return ok;
}

//receive and parse and ... a packet with a control message
int rcv_cntrlmsg(struct tsndrive_t* drivesim,struct cntrlnfo_t * cntrlnfo)
{
        int ok = 0;
        struct rt_pkt_t *rcvd_pkt;
        struct msghdr rcvd_msghdr;
        enum msgtyp_t msg_typ;

        struct pollfd fds[1] = {};
        fds[0].fd = drivesim->rxsckt;
        fds[0].events = POLLIN;

        union dtstmsg_t *dtstmsgs[1] = {NULL};
        int dtstmsgcnt;

        //check for RX-packet
        ok = poll(fds,1,drivesim->cnfg_optns.rcvwndw);
        if (ok <= 0)
                return -1;       //TODO make cycle fitting
        
        //receive paket
        ok = getfreepkt(&(drivesim->pkts),&rcvd_pkt);
        if (ok == 1) {
                printf("Could not get free packet for receiving. \n");
                return 1;       //hardfail
        }
        ok = rcvpkt(drivesim->rxsckt, rcvd_pkt, &rcvd_msghdr);
        if (ok == 1) {
                printf("Receive failed. \n");
                retusedpkt(&(drivesim->pkts),&rcvd_pkt);
                return 1;       //hardfail
        }

        // check ETH-header
        ok = chckethhdr(rcvd_pkt, drivesim->cnfg_optns.rcvaddr, 1);
        if (ok == -1) {
                printf("Check ETH-Header failed. \n");
                retusedpkt(&(drivesim->pkts),&rcvd_pkt);
                return -1;       //continue
        }
        //parse RX-packet
        ok = prspkt(rcvd_pkt, &msg_typ);
        if ((ok == 1) || (msg_typ != CNTRL)) {
                printf("Parsing of received packet failed, or packet not a AXS-packet.\n");
                retusedpkt(&(drivesim->pkts),&rcvd_pkt);
                return -1;       //continue
        }
        ok = chckpkthdrs(rcvd_pkt);
        if (ok == 1) {
                printf("Check Packet-Headers failed. \n");
                retusedpkt(&(drivesim->pkts),&rcvd_pkt);
                return -1;       //continue
        }

        //TODO: might want to check sequence number
        ok = prsdtstmsg(rcvd_pkt, msg_typ, dtstmsgs, &dtstmsgcnt);

        // expected to have only one datasetmessage
        ok = prscntrlmsg(dtstmsgs[0],cntrlnfo);
        retusedpkt(&(drivesim->pkts),&rcvd_pkt);
        return 0;       //success

}

int snd_axsmsg(struct tsndrive_t* drivesim, struct sockaddr_ll *snd_addr, struct axsnfo_t* axsnfo, struct timespec * txtm, uint16_t * seqno)
{
        int ok = 0;
        struct rt_pkt_t *snd_pkt;
        struct msghdr snd_msghdr;
        struct timespec axs_txtime;

        axs_txtime.tv_nsec = 0;
        axs_txtime.tv_sec = 0;
        tmspc_add(&axs_txtime,&axs_txtime, txtm);

        switch(axsnfo->axsID){
        case x:
                break;
        case y:
                inc_tm(&axs_txtime,drivesim->cnfg_optns.sndwndw);
                break;
        case z: 
                inc_tm(&axs_txtime,drivesim->cnfg_optns.sndwndw*2);
                break;
        case s:
                inc_tm(&axs_txtime,drivesim->cnfg_optns.sndwndw*3);
                break;
        default:
                return 1;       //fail
                break;
        }

        //get and fill TX-Packet
        ok = getfreepkt(&(drivesim->pkts),&snd_pkt);       //maybe change to one static packet in thread to avoid competing access to paket store from rx and tx threads
        if (ok == 1){
                printf("Could not get free packet for sending. \n");
                return 1;       //fail
        }
        ok += setpkt(snd_pkt,1,AXS);
        ok += fillaxspkt(snd_pkt,axsnfo,*seqno);
        ok += fillmsghdr(&snd_msghdr,snd_addr,cnvrt_tmspc2int64(&axs_txtime),CLOCK_TAI);
        if (ok != 0){
                printf("Error in filling sending packet or corresponding headers.\n");
                return 1;       //fail
        }
        //send TX-Packet
        ok += sendpkt(drivesim->txsckt,snd_pkt->sktbf,snd_pkt->len,&snd_msghdr);
        if (0 == ok)
                *seqno++;    //sending packet succeded

        //return packet to store
        ok += retusedpkt(&(drivesim->pkts),&snd_pkt);
        return ok;
}

//Real time thread drivesimulation
void *rt_thrd(void *tsndrivesim)
{
	int ok = 0;
        int rcv_ok = 0;
        struct tsndrive_t *drivesim = (struct tsndrive_t *) tsndrivesim;
        struct timespec est;
        struct timespec wkuprcvtm;
        struct timespec txtime;
        

        struct sockaddr_ll snd_addrs[4];
        uint16_t snd_seqno[4] = {0,0,0,0};
        
        struct timespec curtm;
        struct timespec frst_txtime;    //txtime of x-axis, reagrdless if simulated

        struct cntrlnfo_t rcv_cntrlnfo;
        struct axsnfo_t snd_axsnfo;
        double tmstp;

        //init sending address since it will be static
        for (int i = 0; i < drivesim->cnfg_optns.num_axs;i++) {
                ok = fillethaddr(&(snd_addrs[i]), drivesim->cnfg_optns.snd_macs[drivesim->cnfg_optns.frst_axs + i], ETHERTYPE, drivesim->txsckt, drivesim->cnfg_optns.ifname);
        }

        /*sleep this (basetime minus one period) is reached
        or (basetime plus multiple periods) */
        clock_gettime(CLOCK_TAI,&wkuprcvtm);
        clc_est(&wkuprcvtm,&(drivesim->cnfg_optns.basetm), drivesim->cnfg_optns.intrvl_ns, &est);
        frst_txtime = clc_txtm(&est,drivesim->cnfg_optns.sndoffst,SENDINGSTACK_DURATION);
        wkuprcvtm = clc_rcvwkuptm(&est,drivesim->cnfg_optns.rcvoffst,RECEIVINGSTACK_DURATION,APPRECVWAKEUP,MAXWAKEUPJITTER);

        ok = 0;
        while (cmptmspc_Ab4rB(&frst_txtime, &wkuprcvtm)) {
                inc_tm(&frst_txtime,drivesim->cnfg_optns.intrvl_ns);
                ok++;
        }
        if (ok > 0)
                printf("WARNING: Sending time of packets before wakeup for receving packet.\n"
                       "         Answers to received packet will be delayed by %d cycle(s). \n"
                       "         To fix this, adjust network schedule and/or reduce stack calculation time.\n", ok);

        //sleep till first wakeup time
        clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &wkuprcvtm, NULL);
        
        ok = 0;
	int cyclecnt =0;
        //while loop
        while(cyclecnt < 10000){
                clock_gettime(CLOCK_TAI,&curtm);
		printf("Current Time: %lld.%.09ld Cycle: %08d\n",(long long) curtm.tv_sec,curtm.tv_nsec,cyclecnt);
		cyclecnt++;
                
                //receive control message
                rcv_ok = rcv_cntrlmsg(drivesim,&rcv_cntrlnfo);      //TODO: what if more then one packet has arrived within timeframe
                if (rcv_ok > 0){
                        printf("fatal error during receive\n");
                        return NULL; //fail
                }

                if (rcv_ok == 0) {
                        //update enable values
                        ok = axes_updt_enbl(drivesim->axes,drivesim->cnfg_optns.num_axs,&rcv_cntrlnfo);
                }
                
                // calc new new position values and send current positions
                for(int i= 0; i < drivesim->cnfg_optns.num_axs;i++) {
                        //calc new positions and fill sending axs_nfo
                        axs_fineclcpstn(drivesim->axes[i],tmstp,FINEITERATIONS);
                
                        snd_axsnfo.axsID = drivesim->axes[i]->axs;
                        snd_axsnfo.cntrlvl = drivesim->axes[i]->cur_pos;
                        snd_axsnfo.cntrlsw = drivesim->axes[i]->flt;
                        //generate and send packet
                        ok = snd_axsmsg(drivesim, &(snd_addrs[i]), &snd_axsnfo, &frst_txtime, &(snd_seqno[i]));
                        if (ok != 0){
                                printf("fatal error during send\n");
                                return NULL; //fail
                        }
                }

                if (rcv_ok == 0) {
                        //update velocity values
                        ok = axes_updt_setvel(drivesim->axes, drivesim->cnfg_optns.num_axs, &rcv_cntrlnfo);
                }

                //update time
                inc_tm(&wkuprcvtm,drivesim->cnfg_optns.intrvl_ns);
                inc_tm(&frst_txtime,drivesim->cnfg_optns.intrvl_ns);

                //sleep until the next cycle
                clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &wkuprcvtm, NULL);
        }

        return NULL;
}

int main(int argc, char* argv[])
{
        struct tsndrive_t drivesim;
        int ok;

      
        //parse CLI arguments
        evalCLI(argc,argv,&drivesim);

        //init (including real-time)
        ok = init(&drivesim);
        if(ok != 0){
                printf("Initialization failed\n");
                //cleanup
                cleanup(&drivesim);
                return ok;       //fail
        }
        
        //register signal handlers
        signal(SIGTERM, sigfunc);
        signal(SIGINT, sigfunc);

        //start rt-thread   
        /* Create a pthread with specified attributes */
  //      ok = pthread_create(&(drivesim.rt_thrd), &(drivesim.rtthrd_attr), (void*) rt_thrd, (void*)&drivesim);
        rt_thrd((void*)&drivesim);
        if (ok) {
                printf("create pthread failed\n");
                //cleanup
                cleanup(&drivesim);
                return 1;      //fail
        }
 
        /* Join the thread and wait until it is done */
	int ret;
//        ret = pthread_join((sender.rt_thrd), NULL);
        if (ret)
                printf("join pthread failed: %m\n");
        

        //start pmc-thread

        //wait until run is changed? Check how such programms are generally stopped
        while(run){
                sleep(10);
        }

        // cleanup
        ok = cleanup(&drivesim);

        return 0;       //succeded
}