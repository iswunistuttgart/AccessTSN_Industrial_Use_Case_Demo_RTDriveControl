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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "demoapps_common/mk_shminterface.h"

/* ID to identify the axis */
enum axsID_t {
        x,
        y,
        z,
        s,
};

/* struct for the information per axis, this can be either set point or current value */
struct axsnfo_t {
        double cntrlvl;
        bool cntrlsw;
        enum axsID_t axsID;
};

/* main information from the control */
struct cntrlnfo_t {
        struct axsnfo_t x_set;
        struct axsnfo_t y_set;
        struct axsnfo_t z_set;
        struct axsnfo_t s_set;	
	bool spindlebrake;
	bool machinestatus;
	bool estopstatus;
};

/* opens the shared memory (read only) with the information from the control */
struct mk_mainoutput* opnShM_cntrlnfo();

/* opens the shared memory (read write) with the information from the axises */
struct mk_maininput* opnShM_axsnfo();

/* writes the information from one axis to the shared memory */
int wrt_axsinfo2shm(struct axsnfo_t* axsnfo, struct mk_maininput* mk_mainin);

/* gets control information form the shared memory connected to the control */
int rd_shm2cntlinfo(struct mk_mainoutput* mk_mainout, struct cntrlnfo_t* cntrlnfo);

/* closes opened shared memories */
void clsShM();

#endif /* _AXIXSHMHANDLER_H_ */