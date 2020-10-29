// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/*
 * This defines some structs to hold common data.
 */

#ifndef _DATASTRUCTS_H_
#define _DATASTRUCTS_H_

#include <stdint.h>
#include <stdbool.h>

/* ID to identify the axis */
enum axsID_t {
        x = 0,
        y = 1,
        z = 2,
        s = 3,
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

#endif /* _DATASTRUCTS_H_ */