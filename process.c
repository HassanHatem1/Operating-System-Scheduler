#include "headers.h"
#define sharedMemKey 44
#define process_schedulerSHMKey 55

/* Modify this file as needed*/
int remainingtime;
int main(int agrc, char *argv[])
{
    key_t key = ftok("SharedMemoryKeyFile", sharedMemKey); // for shared memory

    int id = atoi(argv[0]); // id of process (to send it in the notification to the scheduler)

    int Arr_Size = atoi(argv[1]) + 1; //+1 because if you have 20 processess id from 1 to 20 not 0 to 19 so it must be size +1

    printf("process with id %d created\n", id); // for testing

    int shmid = shmget(key, Arr_Size * sizeof(int), IPC_CREAT | 0666);
    int *sharedMem = (int *)shmat(shmid, (void *)0, 0); // shared memory for remaining time of the process

    if (sharedMem == (void *)-1)
    {
        perror("Error in attach \n");
        exit(-1);
    }
    key_t key2 = ftok("process_schedulerSHM", process_schedulerSHMKey); // for shared memory
    int prevclkid = shmget(key2, 1 * sizeof(int), IPC_CREAT | 0666);

    initClk();

    // int prevClk = getClk();
    // remainingtime = sharedMem[id];
    // while (remainingtime > 0) ////////////work to do here
    // {
    //     if (prevClk < getClk())
    //     {
    //         printf( "process with id:%d had prevclk:%d and getclk:%d and remainingtime:%d\n",id,prevClk,getClk(),remainingtime);
    //         sharedMem[id] = sharedMem[id] - 1;
    //         remainingtime = remainingtime - 1; // no need for this variable
    //     }
    //     prevClk = getClk();
    // }

    // int prevClk = getClk();
    remainingtime = sharedMem[id];
    printf("remtime of process %d : %d\n", id, sharedMem[id]);
    int *prev = (int *)shmat(shmid, (void *)0, 0); // shared memory for remaining time of the process
    (*prev) = getClk();

    // int currentClk = getClk();

    while (sharedMem[id] > 0)
    {
        // int currentClk = getClk();
        // if (prevClk < currentClk)
        // {
        //     int timeElapsed = currentClk - prevClk;
        //     sharedMem[id] -= timeElapsed;
        //     remainingtime -= timeElapsed;
        //     printf("process with id:%d had prevclk:%d and getclk:%d and remainingtime:%d\n", id, prevClk, currentClk, remainingtime);
        // }
        // prevClk = currentClk;

        while (getClk() == (*prev))
        {
            // one cycle
        }
        while (getClk() == (*prev))
        {
        }

        *prev = getClk();
        sharedMem[id]--;
        printf("process with id:%d  getclk:%d and remainingtime:%d\n", id, getClk(), sharedMem[id]);
    }
    shmdt(sharedMem);
    shmdt(prev);

    destroyClk(false);
    return 0;
}
