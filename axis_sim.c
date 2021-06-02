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
        axs->set_vel = 0;
        axs->enbl = false;
        axs->flt = false;
        axs->max_pos = max_pos;
        axs->max_vel = max_vel;
        axs->min_vel = min_vel;
}

int axes_initreq(struct axis_t* axes[], uint8_t no_axs, enum axsID_t strt_ID)
{
        int a = 0;
        if (NULL == axes)
                return 1;       //fail
        
        switch (strt_ID) {
        case x:
                axs_init(axes[a],x,X_MAX,-X_VEL,X_VEL,0);
                a++;
                if(a == no_axs)
                        break;
        case y:
                axs_init(axes[a],y,Y_MAX,-Y_VEL,Y_VEL,0);
                a++;
                if(a == no_axs)
                        break;
        case z:
                axs_init(axes[a],z,Z_MAX,-Z_VEL,Z_VEL,0);
                a++;
                if(a == no_axs)
                        break;
        case s:
                axs_init(axes[a],s,S_MAX,-S_VEL,S_VEL,0);
                break;
        
        default:
                return 1;       //fail
                break;
        }
        return 0;
}

void axs_clcpstn(struct axis_t* axs, double tmstp)
{
        double new_pos;
        double new_vel;
        double div;
        double tmstp_sq;

        if (axs->enbl != true)
                return;
        //calculate PT2 for velocity
        tmstp_sq = tmstp*tmstp;
        div = tmstp_sq + d_T2*tmstp + TSQUARE;
        new_vel = tmstp_sq*K*axs->set_vel + d_T2*tmstp*axs->cur_vel + 2*tmstp_sq*axs->cur_vel - tmstp_sq*axs->last_vel;
        new_vel = new_vel/div;
        if (new_vel > axs->max_vel)
                new_vel = axs->max_vel;
        if (new_vel < axs->min_vel)
                new_vel = axs->min_vel;

        //update position with new vel; be carful of units (position: mm; vel mm/s; timestep: s)
        new_pos = axs->cur_pos + axs->cur_vel*tmstp + 0.5*(new_vel - axs->cur_vel)*tmstp;              
        if (new_pos > axs->max_pos)
                new_pos = axs->max_pos;         //OPTIONAL: drive fault could be activated if limit is reached
        if (new_pos < -axs->max_pos)
                new_pos = -axs->max_pos;
        
        axs->cur_pos = new_pos;
        axs->last_vel = axs->cur_vel;
        axs->cur_vel = new_vel;
}

void axs_fineclcpstn(struct axis_t* axs, double tmstp, uint32_t iters)
{
        double finetmstp;
        finetmstp = tmstp/iters;
        for (int i = 0; i< iters; i++){
                axs_clcpstn(axs,finetmstp);
        }
}

void axs_enbl(struct axis_t* axs)
{
        axs_clrflt(axs);
        axs->enbl = true;
}

void axs_dsbl(struct axis_t* axs)
{
        axs->enbl = false;
        axs->flt = false;
        axs->set_vel = 0;
        axs_clcpstn(axs,1);
        axs->cur_vel = 0;
}

void axs_clrflt(struct axis_t* axs)
{
        if(axs->enbl != true)
                axs->flt = false;
}

int axes_updt_setvel(struct axis_t* axes[], uint8_t num_axs, const struct cntrlnfo_t *cntrlnfo)
{
        const struct axsnfo_t* set_axsnfo = NULL;
        for (int i = 0; i< num_axs;i++) {
                switch(axes[i]->axs) {
                case x:
                        set_axsnfo = &(cntrlnfo->x_set);
                        break;
                case y:
                        set_axsnfo = &(cntrlnfo->y_set);
                        break;
                case z:
                        set_axsnfo = &(cntrlnfo->z_set);
                        break;
                case s:
                        set_axsnfo = &(cntrlnfo->s_set);
                        break;
                default:
                        return 1;       //fail
                }
                axes[i]->set_vel = set_axsnfo->cntrlvl;
        }
        return 0;
}

/* update enable of all simualted axes */
int axes_updt_enbl(struct axis_t* axes[], uint8_t num_axs, const struct cntrlnfo_t *cntrlnfo)
{
        const struct axsnfo_t* set_axsnfo = NULL;
        for (int i = 0; i< num_axs;i++) {
                switch(axes[i]->axs) {
                case x:
                        set_axsnfo = &(cntrlnfo->x_set);
                        break;
                case y:
                        set_axsnfo = &(cntrlnfo->y_set);
                        break;
                case z:
                        set_axsnfo = &(cntrlnfo->z_set);
                        break;
                case s:
                        set_axsnfo = &(cntrlnfo->s_set);
                        break;
                default:
                        return 1;       //fail
                }

                if(set_axsnfo->cntrlsw > 0) {
                        axs_enbl(axes[i]);
                } else {
                        axs_dsbl(axes[i]);
                }
                if(set_axsnfo->cntrlsw < 0) {
                        axs_ststrtup(axes[i],set_axsnfo->cntrlvl);
                }
        }
        return 0;
}

/* set startup position of axis */
void axs_ststrtup(struct axis_t* axs, double start_pos)
{
        double strtpos;
        strtpos = start_pos;
        if (start_pos > axs->max_pos)
                strtpos = axs->max_pos;
        if (start_pos < -axs->max_pos)
                strtpos = -axs->max_pos;
        axs->cur_pos = strtpos;
}
