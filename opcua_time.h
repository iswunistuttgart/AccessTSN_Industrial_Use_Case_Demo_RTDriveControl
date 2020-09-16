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

#ifndef _OPCUA_TIME_H_
#define _OPCUA_TIME_H_

#include <time.h>
#include <stdint.h>

#define EPOCH_DIFF 11644473600LL

/* convert timespec to ua-time */
uint64_t cnvrt_tmspc2uatm(struct timespec orgtm);

/* convert ua-time to timespec */
struct timespec cnvrt_uatm2tmspc(uint64_t orgtm);



#endif /* _OPCUA_TIME_H_ */