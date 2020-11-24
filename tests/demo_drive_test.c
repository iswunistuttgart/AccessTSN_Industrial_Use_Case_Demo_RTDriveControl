// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/*
 * This uses the simulated axis and interfaces it with the shared memory directly. 
 * To test the simulation of the axis directly without interference from the 
 * communication.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include "../axis_sim.h"
#include "../axisshm_handler.h"
#include "../time_calc.h"

uint8_t run = 1;

struct drivetest_t {
        uint32_t intrvl_ns;
        struct axis_t * axes[4];
        uint8_t num_axs;
        enum axsID_t frst_axs;
        struct mk_mainoutput *txshm;
        struct mk_additionaloutput *atxshm;
        struct mk_maininput *rxshm;
        sem_t* txshm_sem;
        sem_t* atxshm_sem;
        sem_t* rxshm_sem;
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
                " -t [value]           Specifies update-period in microseconds. Default 1 miliseconds.\n"
                " -n [value < 5]       Number of simulated axes. Default 4.\n"
                " -a [index < 4]       Index of the first simulated axis. x = 0, y = 1, z = 2, spindle = 3. Default 0.\n"
                " -h                   Prints this help message and exits\n"
                "\n",
                appname);
}

/* Evaluate CLI-parameters */
void evalCLI(int argc, char* argv[0],struct drivetest_t * drivesim)
{
        int c;
        char* appname = strrchr(argv[0], '/');
        appname = appname ? 1 + appname : argv[0];
        while (EOF != (c = getopt(argc,argv,"ht:b:o:r:w:s:i:n:a:"))) {
                switch(c) {
                case 't':
                        (*drivesim).intrvl_ns = atoi(optarg)*1000;
                        break;
                case 'n':
                        (*drivesim).num_axs = atoi(optarg);
                        break;
                case 'a':
                        (*drivesim).frst_axs = atoi(optarg);
                        break;
                case 'h':
                default:
                        usage(appname);
                        exit(0);
                        break;
                }
        }
        if ((*drivesim).num_axs > 4) {
                printf("Number of simulated Axis to high! Maximum 4.\n");
                exit(0);
        }
        if ((*drivesim).frst_axs > 3) {
                printf("Axis index to high! Maximum 4.\n");
                exit(0);
        }
        if (((*drivesim).frst_axs + (*drivesim).num_axs) > 4) {
                printf("Combination of Axis index and number of simulation Axis to high!\n");
                exit(0);   
        }
}

int init(struct drivetest_t*drivesim)
{
        int ok;

        //open shared memory
        drivesim->txshm = opnShM_cntrlnfo(&drivesim->txshm_sem);
        if(drivesim->txshm == NULL) {
                printf("open of TX sharedmemory failed\n");
                return 1;
        };
        drivesim->atxshm = opnShM_addcntrlnfo(&drivesim->atxshm_sem);
        if(drivesim->atxshm == NULL) {
                printf("open of additional TX sharedmemory failed\n");
                return 1;
        };
        drivesim->rxshm = opnShM_axsnfo(&drivesim->rxshm_sem);
        if(drivesim->txshm == NULL) {
                printf("open of RX sharedmemory failed\n");
                return 1;
        };

        //create correct no of axis
        for  (int i = 0; i < drivesim->num_axs;i++) {
                drivesim->axes[i] = calloc(1,sizeof(struct axis_t));
                if (NULL == drivesim->axes[i])
                return 1;       //fail
        }
        for  (int i = drivesim->num_axs; i < 4;i++) {
                drivesim->axes[i] = NULL;
        }
        ok = axes_initreq(drivesim->axes, drivesim->num_axs, drivesim->frst_axs);
  
}

int cleanup(struct drivetest_t * drivesim)
{
        int ok;
        //close shared memory
        ok += clscntrlShM(&(drivesim->txshm),&drivesim->txshm_sem);
        ok += clsaddcntrlShM(&(drivesim->atxshm),&drivesim->atxshm_sem);
        ok += clsaxsShM(&(drivesim->rxshm),&drivesim->rxshm_sem);

        for (int i = 0;i<4;i++){
                if (drivesim->axes[i]!= NULL) {
                        free(drivesim->axes[i]);
                        drivesim->axes[i] = NULL;
                }
        }
        return ok;  
}

int main(int argc, char* argv[])
{
        struct drivetest_t drivesim;
        int ok;
        drivesim.intrvl_ns = 1000000;
      
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

        int cyclecnt =0;
        struct timespec curtm;
        struct timespec wkuptm;
        struct timespec cntrlrd_tmout;
        struct cntrlnfo_t cntrlnfo;
        struct axsnfo_t axsnfo;
        double tmstp;
        struct timespec axswrt_tmout;
        uint32_t axswrt_tmoutfrac;
        axswrt_tmoutfrac = drivesim.intrvl_ns/6;
        clock_gettime(CLOCK_TAI,&wkuptm);
        //bool instrtup = true;
        tmstp = (double) drivesim.intrvl_ns/1000000000;
        while(run){
                tmspc_cp(&cntrlrd_tmout,&wkuptm);
                inc_tm(&cntrlrd_tmout,drivesim.intrvl_ns/2);
                tmspc_cp(&axswrt_tmout,&wkuptm);
                inc_tm(&axswrt_tmout,axswrt_tmoutfrac*3);
                clock_gettime(CLOCK_TAI,&curtm);
		printf("Current Time: %11d.%.1ld Cycle: %08d\n",(long long) curtm.tv_sec,curtm.tv_nsec,cyclecnt);
		cyclecnt++;
                //get TX values from shared memory
                ok = rd_shm2cntrlinfo(drivesim.txshm, &cntrlnfo, drivesim.txshm_sem, &cntrlrd_tmout);
                
                //if(instrtup)
                //        instrtup = axes_startup(drivesim.rxshm,drivesim.atxshm,drivesim.rxshm_sem,drivesim.atxshm_sem,&cntrlnfo,&cntrlrd_tmout);

                
                //update enable values
                ok = axes_updt_enbl(drivesim.axes,drivesim.num_axs,&cntrlnfo);
                
                
                // calc new new position values and send current positions
                for(int i= 0; i < drivesim.num_axs;i++) {
                        //calc new positions and fill sending axs_nfo
                        axs_fineclcpstn(drivesim.axes[i],tmstp,FINEITERATIONS);
                
                        axsnfo.axsID = drivesim.axes[i]->axs;
                        axsnfo.cntrlvl = drivesim.axes[i]->cur_pos;
                        axsnfo.cntrlsw = drivesim.axes[i]->flt;
                        //update aes shm
                        ok =  wrt_axsinfo2shm(&axsnfo, drivesim.rxshm,drivesim.rxshm_sem,&axswrt_tmout);
                        inc_tm(&axswrt_tmout,axswrt_tmoutfrac);
                }
                printf("UPdates cur Values: x: %f; y: %f; z: %f \n",drivesim.axes[0]->cur_pos,drivesim.axes[1]->cur_pos,drivesim.axes[2]->cur_pos),

                //update velocity values
                ok = axes_updt_setvel(drivesim.axes, drivesim.num_axs, &cntrlnfo);
                                
                inc_tm(&wkuptm,drivesim.intrvl_ns);
                clock_nanosleep(CLOCK_TAI, TIMER_ABSTIME,&wkuptm, NULL);
        }
        ok = cleanup(&drivesim);
        return 0;
}