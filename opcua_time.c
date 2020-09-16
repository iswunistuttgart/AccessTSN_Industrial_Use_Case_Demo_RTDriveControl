// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "opcua_time.h"

uint64_t cnvrt_tmspc2uatm(struct timespec orgtm)
{
        uint64_t newtm;
        newtm = EPOCH_DIFF;
        newtm += orgtm.tv_sec;
        newtm *= 10000000LL;
        newtm += orgtm.tv_nsec / 100;

        return newtm;
}

struct timespec cnvrt_uatm2tmspc(uint64_t orgtm)
{
        struct timespec newtm;
        uint64_t sec;
        uint32_t nsec;
        sec = orgtm/10000000LL - EPOCH_DIFF;
        newtm.tv_sec = (time_t)sec;
        if(sec != newtm.tv_sec) //overflow check
                newtm.tv_sec = (time_t) -1;
        nsec = orgtm%10000000LL * 100;
        newtm.tv_nsec = nsec;

        return newtm;
}