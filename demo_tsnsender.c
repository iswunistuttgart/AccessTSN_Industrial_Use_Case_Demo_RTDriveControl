// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/* Demoapplication to send values taken from shared memory to a drive via TSN
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
#include "packet_handler.h"

uint8_t run = 1;

struct cnfg_optns_t{
        struct timespec basetm;
        uint32_t intrvl_ns;

};

struct tsnsender_t {
        struct cnfg_optns_t cnfg_optns;
        uint32_t dstaddrx;
        uint32_t dstaddry;
        uint32_t dstaddrz;
        uint32_t dstaddrs;
        int rxsckt;
        int txsckt;
        struct mk_mainoutput *txshm;
        struct mk_maininput *rxshm;
        struct pktstore_t pkts;
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
                " -x [IP-address]      Destination IP-address of X-axis (use dot-notation)\n"
                " -y [IP-address]      Destination IP-address of Y-axis (use dot-notation)\n"
                " -z [IP-address]      Destination IP-address of Z-axis (use dot-notation)\n"
                " -s [IP-address]      Destination IP-address of spindle (use dot-notation)\n"
                " -t [value]           Specifies update-period in microseconds. Default 10 seconds.\n"
                " -h                   Prints this help message and exits\n"
                "\n",
                appname);
}

/* Evaluate CLI-parameters */
void evalCLI(int argc, char* argv[0],struct tsnsender_t * sender)
{
        int c;
        char* appname = strrchr(argv[0], '/');
        appname = appname ? 1 + appname : argv[0];
        while (EOF != (c = getopt(argc,argv,"ht:b"))) {
                switch(c) {
                case 'b':
                        cnvrt_dbl2tmspec(atof(optarg), &(sender->cnfg_optns.basetm));
                        break;
                case 't':
                        (*sender).cnfg_optns.intrvl_ns = atoi(optarg)*1000;
                        break;
                case 'h':
                default:
                        usage(appname);
                        exit(0);
                        break;
                }
        }
}

// open socket
// close socket

//initialization
int init(struct tsnsender_t *sender)
{
        int ok = 0;
        struct sched_param param;
        //open send socket

        //open recv socket

        //open shared memory

        //allocate memory for packets
        ok += initpktstrg(&(sender->pkts),5);

        //prefault stack/heap

        // ### setup rt_thread
        //Lock memory
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

        //setup pmc-thread (optional)

        return ok;
}

//End execution with cleanup
int cleanup(struct tsnsender_t *sender)
{
        int ok = 0;
        //stop threads

        //close rx socket

        //close tx socket

        //close shared memory

        //free allocated memory for packets
        ok += destroypktstrg(&(sender->pkts));

        return ok;
}

//Real time thread
void *rt_thrd(void *tsnsender)
{
	struct tsnsender_t *sender;
        struct timespec nxtprd;
        struct timespec curtm;
	sender = (struct tsnsender_t *) tsnsender;
                
        /*sleep this (basetime minus one period) is reached
        or (basetime plus multiple periods) */
        clock_gettime(CLOCK_TAI,&curtm);
        clc_est(&curtm,&(sender->cnfg_optns.basetm), sender->cnfg_optns.intrvl_ns, &nxtprd);
        clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &nxtprd, NULL);
        //TODO define how this timestamp is interpredet as TXTIME or Wakeuptime for application, resulting offset needs to be added
	int cyclecnt =0;
        //while loop
        while(cyclecnt < 1000000){
                
                clock_gettime(CLOCK_TAI,&curtm);
		printf("Current Time: %11d.%.1ld Cycle: %08d\n",(long long) curtm.tv_sec,curtm.tv_nsec,cyclecnt);
		cyclecnt++;

                //check for RX-packet

                //parse RX-packet

                //write RX values to shared memory

                //get TX values from shared memory

                //create TX-Packet
                
                //calculate TxTime-Stamp

                //send TX-Packet

                //update time
                inc_prd(&nxtprd,sender->cnfg_optns.intrvl_ns);

                //sleep until the next cycle
                clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME, &nxtprd, NULL);
        }

        return NULL;
}


int main(int argc, char* argv[])
{
        struct tsnsender_t sender;
        int ok;

        //set standard values
        
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
        ok = pthread_create(&(sender.rt_thrd), &(sender.rtthrd_attr), (void*) rt_thrd, NULL);
        if (ok) {
                printf("create pthread failed\n");
                //cleanup
                cleanup(&sender);
                return 1;      //fail
        }
 
        /* Join the thread and wait until it is done */
	int ret;
        ret = pthread_join((sender.rt_thrd), NULL);
        if (ret)
                printf("join pthread failed: %m\n");
        

        //start pmc-thread

        // cleanup
        ok = cleanup(&sender);

        return 0;       //succeded
}
