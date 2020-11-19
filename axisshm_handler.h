// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/*
 * This interfaces with the shared memory and gets/sets variables for the axis.
 */

#ifndef _AXISSHMHANDLER_H_
#define _AXISSHMHANDLER_H_

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include "demoapps_common/mk_shminterface.h"
#include "datastructs.h"

/* opens the shared memory (read only) with the information from the control */
struct mk_mainoutput* opnShM_cntrlnfo(sem_t** sem);

/* opens the shared memory (read write) with the information from the axises */
struct mk_maininput* opnShM_axsnfo(sem_t** sem);

/* writes the information from one axis to the shared memory */
int wrt_axsinfo2shm(struct axsnfo_t* axsnfo, struct mk_maininput* mk_mainin,sem_t* mainin_sem, struct timespec* tmout);

/* gets control information form the shared memory connected to the control */
int rd_shm2cntlinfo(struct mk_mainoutput* mk_mainout, struct cntrlnfo_t* cntrlnfo, sem_t* mainout_sem, struct timespec* tmout);

/* closes opened shared memories */
int clscntrlShM(struct mk_mainoutput** mk_mainout, sem_t** sem);
int clsaxsShM(struct mk_maininput** mk_mainin, sem_t** sem);

#endif /* _AXIXSHMHANDLER_H_ */
