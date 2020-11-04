// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

/*
 * This simulates an axis with a simple PT2 filter. It is assumed that the 
 * minimum value of the axis is zero.
 */

#ifndef _AXIS_SIM_H_
#define _AXIS_SIM_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "datastructs.h"

#define K 1     // K-Factor
#define T 1     // Timefactor
#define d 0.5   // Damping


#define TSQUARE (T*T)
#define d_T2 (d*T*2)


struct axis_t {
        enum axsID_t axs;
        double max_pos;
        double min_vel;
        double max_vel;
        double cur_pos;
        double cur_vel;
        bool enbl;
        bool flt;
};

/* initialization of axis */
void axs_init(struct axis_t* axs, enum axsID_t axsID, double max_pos, double min_vel, double max_vel, double start_pos);

/* calculate new position, one timestep */
void axs_clcpstn(struct axis_t* axs, double set_vel, double tmstp);

/* enable axis and clears fault if necessary/and possible*/
void axs_enbl(struct axis_t* axs);

/* disable axis */
void axs_dsbl(struct axis_t* axs);

/* clear fault can only be done if axis is disabled*/
void clr_flt(struct axis_t* axs);


#endif /* _AXIS_SIM_H_ */