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
#include "demohelpers.h"

uint8_t run = 1;
struct tsnsender_t {
        uint16_t dstp;
        uint32_t period;
        uint32_t srcaddr;
        uint32_t dstaddrx;
        uint32_t dstaddry;
        uint32_t dstaddrz;
        uint32_t dstaddrs;
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
                " -t [value]           Specifies update-period in milliseconds. Default 10 seconds.\n"
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
        while (EOF != (c = getopt(argc,argv,"ht:d"))) {
                switch(c) {
                case 'd':
                        (*sender);
                        break;
                case 't':
                        (*sender).period = atoi(optarg)*1000;
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

int main(int argc, char* argv[])
{
        struct tsnsender_t sender;
        
        evalCLI(argc,argv,&sender);

        //init (including real-time)

        //register signal handlers
        signal(SIGTERM, sigfunc);
        signal(SIGINT, sigfunc);

        // mainloop
        while(run) {

              
        }

        // cleanup

        return 0;
}