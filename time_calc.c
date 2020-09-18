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

//increase period by timeinterval
void inc_prd(struct timespec *prd, uint32_t intrvl)
{
        prd->tv_nsec += intrvl;
 
        while (prd->tv_nsec >= NSEC_IN_SEC) {
                //timespec nsec overflow
                prd->tv_sec++;
                prd->tv_nsec -= NSEC_IN_SEC;
        }
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

void cnvrt_dbl2tmspec(uint64_t orgtm, struct timespec *newtm)
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