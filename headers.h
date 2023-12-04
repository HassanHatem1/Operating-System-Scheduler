#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
// ---------------------------process contorl block structure --------------------------------
typedef struct
{
    int id;
    int pid;
    int arrival;
    int brust;
    int finish;
    int running;
    int stop;
    int priority;
    int start;
    int wait;
} PCB;

void setPCB(PCB *pcb, int nID, int nPID, int nArrival, int nBurst, int nFinish, int nRunning, int nStop, int nPriority, int nStart, int nWait)
{
    pcb->id = nID;
    pcb->pid = nPID;
    pcb->arrival = nArrival;
    pcb->brust = nBurst;
    pcb->finish = nFinish;
    pcb->running = nRunning;
    pcb->stop = nStop;
    pcb->priority = nPriority;
    pcb->start = nStart;
    pcb->wait = nWait;
}
//-----------------------------------------------------------------------------------------

// ---------------------------Queue implementation for RR --------------------------------
