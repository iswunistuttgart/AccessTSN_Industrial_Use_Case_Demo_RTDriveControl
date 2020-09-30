// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/*
 * This specifies some helper functions to convert time between the OPC UA time
 * format and the timespec format. This is used to convert the timestamps used
 * in the OPC UA network messages.
 */

#ifndef _TIME_CALC_H_
#define _TIME_CALC_H_

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define OPC_EPOCH_DIFF 11644473600LL
#define NSEC_IN_SEC 1000000000LL

/* convert timespec to ua-time */
uint64_t cnvrt_tmspc2uatm(struct timespec orgtm);

/* convert ua-time to timespec */
struct timespec cnvrt_uatm2tmspc(uint64_t orgtm);

/* increase time by timeinterval */
void inc_tm(struct timespec *tm, uint32_t intrvl);

/* decrease time by timeinterval */
void dec_tm(struct timespec *tm, uint32_t intrvl);

/* convert timespec to uint64 (in nanosec) */
uint64_t cnvrt_tmspec2int64(struct timespec *orgtm);

/* convert uint64 (nanosec) to timespec */
void cnvrt_int642tmspec(uint64_t orgtm, struct timespec *newtm);

/* convert double (sec) to timespec */
void cnvrt_dbl2tmspec(double orgtm, struct timespec *newtm);

/* compare two timespecs */
bool cmptmspc_Ab4rB(const struct timespec *A, const struct timespec *B);

/* add Timespecs (A+B) */
void tmspc_add(struct timespec *res, const struct timespec *A, const struct timespec *B);

/* substract Timespecs (A-b) */
void tmspc_sub(struct timespec *res, const struct timespec *A, const struct timespec *B);

/* calc epoch start time */
void clc_est(const struct timespec *curtm, const struct timespec *basetm, uint32_t intrvl, struct timespec *est);

/* calc next txtime a.k.a. when the next send frame should leave the network interface */
struct timespec clc_txtm(const struct timespec * est, uint32_t sndoffst, uint32_t sndstckclc);

/* calc the wakeup time for the sending thread */
struct timespec clc_sndwkuptm(const struct timespec *txtime, uint32_t sndappclc, uint32_t maxwkupjttr);

/* calc the wakeup time for the receiving thread */
struct timespec clc_rcvwkuptm(const struct timespec * est, uint32_t rcvoffst, uint32_t rcvstckclc, uint32_t rcvappclc, uint32_t maxwkupjttr);

#endif /* _TIME_CALC_H_ */