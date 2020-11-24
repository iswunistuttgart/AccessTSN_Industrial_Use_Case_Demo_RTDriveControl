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
#define T 0.0001     // Timefactor
#define d 1   // Damping

#define X_MAX 300       //in mm
#define X_VEL 60    //in mm/s
#define Y_MAX 300       //in mm
#define Y_VEL 60       //in mm/s
#define Z_MAX 300       //in mm
#define Z_VEL 60        //in mm/s
#define S_MAX 300       
#define S_VEL 10        

#define FINEITERATIONS 10

#define TSQUARE (T*T)
#define d_T2 (d*T*2)


struct axis_t {
        enum axsID_t axs;
        double max_pos;
        double min_vel;
        double max_vel;
        double cur_pos;
        double cur_vel;
        double set_vel;
        double last_vel;
        bool enbl;
        bool flt;
};

/* initialization of axis */
void axs_init(struct axis_t* axs, enum axsID_t axsID, double max_pos, double min_vel, double max_vel, double start_pos);

/* init requested axis */
int axes_initreq(struct axis_t* axes[], uint8_t no_axs, enum axsID_t strt_ID);

/* calculate new position, one timestep */
void axs_clcpstn(struct axis_t* axs, double tmstp);

/* calculate new position, multi_small timesteps */
void axs_fineclcpstn(struct axis_t* axs, double tmstp, uint32_t iters);

/* enable axis and clears fault if necessary/and possible*/
void axs_enbl(struct axis_t* axs);

/* disable axis */
void axs_dsbl(struct axis_t* axs);

/* clear fault can only be done if axis is disabled*/
void axs_clrflt(struct axis_t* axs);

/* update set velocities of all simualted axes */
int axes_updt_setvel(struct axis_t* axes[], uint8_t num_axs, const struct cntrlnfo_t *cntrlnfo);

/* update enable of all simualted axes */
int axes_updt_enbl(struct axis_t* axes[], uint8_t num_axs, const struct cntrlnfo_t *cntrlnfo);

/* set startup position of axis */
void axs_ststrtup(struct axis_t* axs, double start_pos);


#endif /* _AXIS_SIM_H_ */
