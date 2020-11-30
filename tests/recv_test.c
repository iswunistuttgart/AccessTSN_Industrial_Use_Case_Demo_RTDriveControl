#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include "../packet_handler.h"


#define NIC "ens5"
#define cycle 1000      //ms
uint8_t run = 1;

/* signal handler */
void sigfunc(int sig)
{
        switch(sig)
        {
        case SIGINT:
                if(run)
                        run = 0;
                else
                        exit(0);
                break;
        case SIGTERM:
                run = 0;
                break;        
        }
}

int main(int argc, char* argv[])
{
        char dstaddr[ETH_ALEN];
        char mc1addr[ETH_ALEN] = {0x02,0xAC,0xCE,0x55,0x00,0x01};
        char mc2addr[ETH_ALEN] = {0x03,0xAC,0xCE,0x55,0x00,0x02};
        int ok;
        int fd;
        char ifname[IFNAMSIZ] = NIC;
        char *mac_addrs[3];
        mac_addrs[0] = dstaddr;
        mac_addrs[1] = mc1addr;
        mac_addrs[2] = mc2addr;
        int no_macs = 3;

        //set standard values
        dstaddr[0] = 0x01;
        dstaddr[1] = 0xAC;
        dstaddr[2] = 0xCE;
        dstaddr[3] = 0x55;
        dstaddr[4] = 0x00;
        dstaddr[5] = 0x00;
                       
        //register signal handlers
        signal(SIGTERM, sigfunc);
        signal(SIGINT, sigfunc);

        fd = opnrxsckt(ifname, mac_addrs, no_macs);
        if (fd < 0) {
                printf("RX Socket open failed. Err: %d\n",fd);
                return 1;
        }

        struct pollfd fds[1] = {};
        fds[0].fd = fd;
        fds[0].events = POLLIN;

        struct rt_pkt_t* pkt;
        ok = createpkt(&pkt);
        struct msghdr rcvmsg_hdr;

        //wait until run is changed? Check how such programms are generally stopped
        while(run){
                usleep(cycle * 1000);
                ok = poll(fds, 1, cycle/10);
                if (ok <= 0)
                        continue;
                
                ok =  rcvpkt(fd, pkt, &rcvmsg_hdr); 

                //print packet
                printf("Recvd Packet: ");
                for (int i = 0;i<200;i++)
                        printf("%02x",pkt->sktbf[i]);
                printf("\n");

                //check ethhdr
                ok = chckethhdr(pkt, mac_addrs, no_macs);
                printf("Result of Check ETH HDR: %d\n",ok);

                /* invalid since receiving raw packet
                //ckech msg hgdr
                ok = chckmsghdr(&rcvmsg_hdr,dstaddr,ETHERTYPE);
                printf("Result of Check MSG dstaddr: %d\n",ok);
                ok = chckmsghdr(&rcvmsg_hdr,mc1addr,ETHERTYPE);
                printf("Result of Check MSG mc1addr: %d\n",ok);
                ok = chckmsghdr(&rcvmsg_hdr,mc2addr,ETHERTYPE);
                printf("Result of Check MSG mc2addr: %d\n",ok);
                */
                
                //parse packet
                enum msgtyp_t messagetype;
                ok = prspkt(pkt, &messagetype);
                printf("Result of Parse paket: %d, Messagetype: %d \n",ok,messagetype);

                //check headers
                ok = chckpkthdrs(pkt);
                printf("Result of Check pkt headers: %d\n",ok);

                //parse datasetmesages
                union dtstmsg_t *datasetmsgs[3] = {NULL,NULL,NULL};
                int datasetmsgcount;
                ok = prsdtstmsg(pkt, messagetype, datasetmsgs, &datasetmsgcount);
                printf("Result of Parse datasetmessages: %d, Datamsgcount: %d \n",ok,datasetmsgcount);
                
        }

        // cleanup
        close(fd);
        destroypkt(pkt);

        return 0;       //succeded
}