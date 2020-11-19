// SPDX-License-Identifier: (MIT)
/*
 * Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
 * Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
 */

#include "axisshm_handler.h"

struct mk_mainoutput* opnShM_cntrlnfo(sem_t** sem)
{
        int fd;
        bool init = false;
        int mpflg = PROT_READ;
        struct mk_mainoutput* shm;
        int semflg = 0;
        fd = shm_open(MK_MAINOUTKEY, O_RDONLY, 0666);
        if ((fd == -1) && (errno == ENOENT)){
                init = true;
                fd = shm_open(MK_MAINOUTKEY, O_RDWR|O_CREAT,0666);
                mpflg = PROT_READ | PROT_WRITE;
                semflg = O_CREAT;
        }
        if (fd == -1) {
                perror("SHM Open failed");
                return(NULL);
        }
        ftruncate(fd,sizeof(struct mk_mainoutput));
        shm = mmap(NULL, sizeof(struct mk_mainoutput), mpflg , MAP_SHARED, fd, 0);
        if (MAP_FAILED == shm) {
                perror("SHM Map failed");
                shm = NULL;
                if(init)
                        shm_unlink(MK_MAINOUTKEY);
        }
        *sem = sem_open(MK_MAINOUTKEY,semflg,0666,0);
        if (*sem == SEM_FAILED) {
                perror("Semaphore open failed");
                munmap(shm, sizeof(struct mk_mainoutput));
                return(NULL);
        }
        if(init) {
                memset(shm,0,sizeof(struct mk_mainoutput));
                mprotect(shm, sizeof(struct mk_mainoutput),PROT_READ);
                sem_post(*sem);
        }
        return shm;
}


struct mk_maininput* opnShM_axsnfo(sem_t** sem)
{
        int fd;
        struct mk_maininput* shm;
        fd = shm_open(MK_MAININKEY, O_RDWR | O_CREAT, 0666);
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
        *sem = sem_open(MK_MAININKEY,O_CREAT,0666,0);
        if(*sem == SEM_FAILED) {
                perror("Semaphore open failed");
                munmap(shm,sizeof(struct mk_maininput));
                shm_unlink(MK_MAININKEY);
                return(NULL);
        }
        memset(shm,0,sizeof(struct mk_maininput));
        sem_post(*sem);
        return shm;
}


int wrt_axsinfo2shm(struct axsnfo_t* axsnfo, struct mk_maininput* mk_mainin, sem_t* mainin_sem, struct timespec* tmout)
{
        int ok;
        if ((NULL == mk_mainin) || (NULL == axsnfo))
                return 1;       //fail
        ok = sem_timedwait(mainin_sem,tmout);
        if((ok == -1) && (errno == ENOENT))
                return 2;       //timedout
        if (ok == -1)
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
        sem_post(mainin_sem);
        return 0;       //succeded
}


int rd_shm2cntlinfo(struct mk_mainoutput* mk_mainout, struct cntrlnfo_t* cntrlnfo, sem_t* mainout_sem, struct timespec* tmout)
{
        int ok;
        if ((NULL == cntrlnfo) || (NULL == mk_mainout))
                return 1;       //fail
        ok = sem_timedwait(mainout_sem,tmout);
        if((ok == -1) && (errno == ENOENT))
                return 2;       //timedout
        if (ok == -1)
                return 1;       //fail
        sem_post(mainout_sem);
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


int clscntrlShM(struct mk_mainoutput** mk_mainout, sem_t** sem)
{
        //not unlinking shared memroy because for rhe control SHm this is only reader
        int ok;
        ok = munmap(*mk_mainout,sizeof(struct mk_mainoutput));
        if (ok < 0)
                return ok;
        *mk_mainout = NULL;
        ok = sem_close(*sem);
        if (ok < 0)
                return ok;
        *sem = NULL;
        return ok;
}

int clsaxsShM(struct mk_maininput** mk_mainin, sem_t** sem)
{
        int ok;
        ok = munmap(*mk_mainin,sizeof(struct mk_maininput));
        if (ok < 0)
                return ok;
        *mk_mainin = NULL;
        ok = sem_close(*sem);
        if (ok < 0)
                return ok;
        *sem = NULL;
        ok =+ shm_unlink(MK_MAININKEY);
        return ok;
}