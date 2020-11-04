// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "axis_sim.h"

void axs_init(struct axis_t* axs, enum axsID_t axsID, double max_pos, double min_vel, double max_vel, double start_pos)
{
        if (NULL == axs)
                return;
        axs->axs = axsID;
        axs->cur_pos = start_pos;
        axs->cur_vel = 0;
        axs->enbl = false;
        axs->flt = false;
        axs->max_pos = max_pos;
        axs->max_vel = max_vel;
        axs->min_vel = min_vel;
}

void axs_clcpstn(struct axis_t* axs, double set_vel, double tmstp)
{
        double new_pos;
        double new_vel;

        if (axs->enbl != true)
                return;

        new_vel = 1/(TSQUARE/(tmstp*tmstp)+d_T2/tmstp + 1) * (K * set_vel - axs->cur_vel) + axs->cur_vel;       //Check if klammer passt
        if (new_vel > axs->max_vel)
                new_vel = axs->max_vel;
        if (new_vel < axs->min_vel)
                new_vel = axs->min_vel;

        new_pos = axs->cur_pos + axs->cur_vel*tmstp + 0.5 * new_vel * tmstp*tmstp;              //check last term 
        if (new_pos > axs->max_pos)
                new_pos = axs->max_pos;                                                         //check is drive fault should be activated
        if (new_pos < 0)
                new_pos = 0;
        
        axs->cur_pos = new_pos;
        axs->cur_vel = new_vel;
}

void axs_enbl(struct axis_t* axs)
{
        clr_flt(axs);
        axs->enbl = true;
}


void axs_dsbl(struct axis_t* axs)
{
        axs->enbl = false;
        axs->flt = false;
        axs_clcpstn(axs,0,1);
        axs->cur_vel = 0;
}

void clr_flt(struct axis_t* axs)
{
        if(axs->enbl != true)
                axs->flt = false;
}