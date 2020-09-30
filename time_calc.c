// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "time_calc.h"

uint64_t cnvrt_tmspc2uatm(struct timespec orgtm)
{
        uint64_t newtm;
        newtm = OPC_EPOCH_DIFF;
        newtm += orgtm.tv_sec;
        newtm *= NSEC_IN_SEC;
        newtm += orgtm.tv_nsec / 100;

        return newtm;
}

struct timespec cnvrt_uatm2tmspc(uint64_t orgtm)
{
        struct timespec newtm;
        uint64_t sec;
        uint32_t nsec;
        sec = orgtm/NSEC_IN_SEC - OPC_EPOCH_DIFF;
        newtm.tv_sec = (time_t)sec;
        if(sec != newtm.tv_sec) //overflow check
                newtm.tv_sec = (time_t) -1;
        nsec = orgtm%NSEC_IN_SEC * 100;
        newtm.tv_nsec = nsec;

        return newtm;
}

/*increase time by timeinterval*/
void inc_tm(struct timespec *tm, uint32_t intrvl)
{
        tm->tv_nsec += intrvl;
 
        while (tm->tv_nsec >= NSEC_IN_SEC) {
                //timespec nsec overflow
                tm->tv_sec++;
                tm->tv_nsec -= NSEC_IN_SEC;
        }
}

/* decrease time by timeinterval */
void dec_tm(struct timespec *tm, uint32_t intrvl)
{
        while (intrvl > tm->tv_nsec) {
                //timespec nsec overflow
                tm->tv_sec--;
                tm->tv_nsec += NSEC_IN_SEC;
        }
        tm->tv_nsec -= intrvl;
}

uint64_t cnvrt_tmspec2int64(struct timespec *orgtm)
{
        return orgtm->tv_sec * NSEC_IN_SEC + orgtm->tv_nsec;
}

void cnvrt_int642tmspec(uint64_t orgtm, struct timespec *newtm)
{
        newtm->tv_nsec = (uint32_t)(orgtm % NSEC_IN_SEC);
        newtm->tv_sec = orgtm / NSEC_IN_SEC;
}

void cnvrt_dbl2tmspec(double orgtm, struct timespec *newtm)
{
        newtm->tv_sec = (time_t)orgtm;
        newtm->tv_nsec = (orgtm - newtm->tv_sec)* NSEC_IN_SEC;
}

bool cmptmspc_Ab4rB(const struct timespec *A, const struct timespec *B)
{
        if(A->tv_sec == B->tv_sec)
                return (A->tv_nsec < B->tv_nsec);
        return (A->tv_sec < B->tv_sec);
}

void tmspc_add(struct timespec *res, const struct timespec *A, const struct timespec *B)
{
        res->tv_sec = A->tv_sec + B->tv_sec;
        res->tv_nsec = A->tv_nsec + B->tv_nsec;
        while (res->tv_nsec >= NSEC_IN_SEC){
                //timespec nsec overflow
                res->tv_sec++;
                res->tv_nsec -= NSEC_IN_SEC;
        }
}

void tmspc_sub(struct timespec *res, const struct timespec *A, const struct timespec *B)
{
        if(A->tv_nsec < B->tv_nsec){
                res->tv_sec = A->tv_sec - B->tv_sec -1;
                res->tv_nsec = NSEC_IN_SEC + A->tv_nsec - B->tv_nsec;
        } else {
                res->tv_nsec = A->tv_nsec - B->tv_nsec;
                res->tv_sec = A->tv_sec - B->tv_sec;
        }
}

void clc_est(const struct timespec *curtm, const struct timespec *basetm, uint32_t intrvl, struct timespec *est)
{
        struct timespec tm;
        uint64_t tm_ns;
        uint64_t cycl;

        //check if basetime still in in the future
        tm = *curtm;
        inc_prd(&tm,intrvl);
        if(cmptmspc_Ab4rB(&tm,basetm)){
                est->tv_sec = basetm->tv_sec;
                est->tv_nsec = basetm->tv_nsec;
                return;
        }
        
        //calculate epoch start time since basetime is in the past
        //calc cycles sind basetime
        tmspc_sub(&tm,curtm,basetm);
        tm_ns = cnvrt_tmspec2int64(&tm);
        cycl = tm_ns/intrvl + 1;        //increase by one to account for fraction
        //cals est from cycles and basetime
        tm_ns = cycl*intrvl;
        cnvrt_int642tmspec(tm_ns,&tm);
        tmspc_add(est,basetm,&tm);
}

/* calc next txtime a.k.a. when the next send frame should leave the network interface */
struct timespec clc_txtm(const struct timespec * est, uint32_t sndoffst, uint32_t sndstckclc)
{
        struct timespec txtime;
        txtime.tv_nsec = 0;
        txtime.tv_sec = 0;
        tmspc_add(&txtime,&txtime, est);
        inc_tm(&txtime,sndoffst);
        dec_tm(&txtime,sndstckclc);
        return(txtime);
}

/* calc the wakeup time for the sending thread */
struct timespec clc_sndwkuptm(const struct timespec *txtime, uint32_t sndappclc, uint32_t maxwkupjttr)
{
        struct timespec sndwkuptm;
        sndwkuptm.tv_sec = 0;
        sndwkuptm.tv_nsec = 0;
        tmspc_add(&sndwkuptm,&sndwkuptm,txtime);
        dec_tm(&sndwkuptm,sndappclc+maxwkupjttr);
        return(sndwkuptm);
}

/* calc the wakeup time for the receiving thread */
struct timespec clc_rcvwkuptm(const struct timespec * est, uint32_t rcvoffst, uint32_t rcvstckclc, uint32_t rcvappclc, uint32_t maxwkupjttr)
{
        struct timespec rcvwkuptm;
        rcvwkuptm.tv_sec = 0;
        rcvwkuptm.tv_nsec = 0;
        tmspc_add(&rcvwkuptm,&rcvwkuptm,est);
        inc_tm(&rcvwkuptm,rcvoffst+rcvstckclc+maxwkupjttr);
        dec_tm(&rcvwkuptm,rcvappclc);
        return(rcvwkuptm);
}