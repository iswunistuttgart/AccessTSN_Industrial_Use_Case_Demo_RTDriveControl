// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "axisshm_handler.h"

struct mk_mainoutput* opnShM_cntrlnfo()
{
        int fd;
        struct mk_mainoutput* shm;
        fd = shm_open(MK_MAINOUTKEY, O_RDONLY | O_CREAT, 700);
        if (fd == -1) {
                perror("SHM Open failed");
                return(NULL);
        }
        ftruncate(fd,sizeof(struct mk_mainoutput));
        shm = mmap(NULL, sizeof(struct mk_mainoutput), PROT_READ , MAP_SHARED, fd, 0);
        if (MAP_FAILED == shm) {
                perror("SHM Map failed");
                shm = NULL;
                shm_unlink(MK_MAINOUTKEY);
        }
        return shm;
}


struct mk_maininput* opnShM_axsnfo()
{
        int fd;
        struct mk_maininput* shm;
        fd = shm_open(MK_MAININKEY, O_RDWR | O_CREAT, 700);
        if (fd == -1) {
                perror("SHM Open failed");
                return(NULL);
        }
        ftruncate(fd,sizeof(struct mk_maininput));
        shm = mmap(NULL, sizeof(struct mk_maininput), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (MAP_FAILED == shm) {
                perror("SHM Map failed");
                shm = NULL;
                shm_unlink(MK_MAININKEY);
        }
        return shm;
}


int wrt_axsinfo2shm(struct axsnfo_t* axsnfo, struct mk_maininput* mk_mainin)
{
        if ((NULL == mk_mainin) || (NULL == axsnfo))
                return 1;       //fail
        switch(axsnfo->axsID){
        case x:
                mk_mainin->xpos_cur = axsnfo->cntrlvl;
                mk_mainin->xfault =axsnfo->cntrlsw;
                break;
        case y:
                mk_mainin->ypos_cur = axsnfo->cntrlvl;
                mk_mainin->yfault =axsnfo->cntrlsw;
                break;
        case z:
                mk_mainin->zpos_cur = axsnfo->cntrlvl;
                mk_mainin->zfault =axsnfo->cntrlsw;
                break;
        default:
                return 1;       //fail
        }
        return 0;       //succeded
}


int rd_shm2cntlinfo(struct mk_mainoutput* mk_mainout, struct cntrlnfo_t* cntrlnfo)
{
        if ((NULL == cntrlnfo) || (NULL == mk_mainout))
                return 1;       //fail
        cntrlnfo->x_set.cntrlvl = mk_mainout->xvel_set;
        cntrlnfo->x_set.cntrlsw = mk_mainout->xenable;
        cntrlnfo->x_set.axsID = x;
        cntrlnfo->y_set.cntrlvl = mk_mainout->yvel_set;
        cntrlnfo->y_set.cntrlsw = mk_mainout->yenable;
        cntrlnfo->y_set.axsID = y;
        cntrlnfo->z_set.cntrlvl = mk_mainout->zvel_set;
        cntrlnfo->z_set.cntrlsw = mk_mainout->zenable;
        cntrlnfo->z_set.axsID = z;
        cntrlnfo->s_set.cntrlvl = mk_mainout->spindlespeed;
        cntrlnfo->s_set.cntrlsw = mk_mainout->spindleenable;
        cntrlnfo->s_set.axsID = s;
        cntrlnfo->spindlebrake = mk_mainout->spindlebrake;
        cntrlnfo->machinestatus = mk_mainout->machinestatus;
        cntrlnfo->estopstatus = mk_mainout->estopstatus;

        return 0;       //succeded
}


void clscntrlShM(struct mk_mainoutput** mk_mainout)
{
        shm_unlink(MK_MAINOUTKEY);
        mk_mainout = NULL;
}

void clsaxsShM(struct mk_maininput** mk_mainin)
{
       shm_unlink(MK_MAININKEY);
       mk_mainin = NULL;
}