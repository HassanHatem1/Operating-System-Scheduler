#include "headers.h"
#define sharedMemKey 44
/* Modify this file as needed*/
int remainingtime;
int main(int agrc, char *argv[])
{
    initClk();
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

    int prevClk = getClk();
    remainingtime = sharedMem[id];
    while (remainingtime > 0) ////////////work to do here
    {
        if (prevClk != getClk())
        {
            sharedMem[id] = sharedMem[id] - 1;
            remainingtime = remainingtime - 1; // no need for this variable
        }
        prevClk = getClk();
    }

    destroyClk(false);
    return 0;
}
