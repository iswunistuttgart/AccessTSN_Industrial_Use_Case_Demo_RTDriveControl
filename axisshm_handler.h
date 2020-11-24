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

/* opens the shared memory (read only) with the additional information from the control */
struct mk_additionaloutput* opnShM_addcntrlnfo(sem_t** sem);

/* opens the shared memory (read write) with the information from the axises */
struct mk_maininput* opnShM_axsnfo(sem_t** sem);

/* writes the information from one axis to the shared memory */
int wrt_axsinfo2shm(struct axsnfo_t* axsnfo, struct mk_maininput* mk_mainin,sem_t* mainin_sem, struct timespec* tmout);

/* read the information from one axis to the shared memory */
int rd_shm2axscntrlinfo(struct mk_maininput* mk_mainin,struct cntrlnfo_t* cntrlnfo, sem_t* mainin_sem, struct timespec* tmout);

/* gets control information form the shared memory connected to the control */
int rd_shm2cntrlinfo(struct mk_mainoutput* mk_mainout, struct cntrlnfo_t* cntrlnfo, sem_t* mainout_sem, struct timespec* tmout);

/* gets control information form the shared memory connected to the control */
int rd_shm2addcntrlinfo(struct mk_additionaloutput* mk_addout, struct cntrlnfo_t* cntrlnfo, sem_t* addout_sem, struct timespec* tmout);

/* checks if axes are still in startup-phase and therefore not in start position and setst the control values accordingly*/
//bool axes_startup(struct mk_maininput* mk_mainin, struct mk_additionaloutput* mk_addout,sem_t* mainin_sem,sem_t* addout_sem,struct cntrlnfo_t* cntrlnfo,struct timespec* tmout);

/* closes opened shared memories */
int clscntrlShM(struct mk_mainoutput** mk_mainout, sem_t** sem);
int clsaddcntrlShM(struct mk_additionaloutput** mk_addout, sem_t** sem);
int clsaxsShM(struct mk_maininput** mk_mainin, sem_t** sem);

#endif /* _AXIXSHMHANDLER_H_ */
