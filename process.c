#include "headers.h"
/* Modify this file as needed*/
int remainingtime;
int main(int agrc, char *argv[])
{
    initClk();
    key_t key = ftok("keyfile", 44); // for shared memory
    int id = atoi(argv[0]); // id of process (to send it in the notification to the scheduler)
    int Arr_Size=atoi(argv[1])+1; //+1 because if you have 20 processess id from 1 to 20 not 0 to 19 so it must be size +1
    int shmid = shmget(key,Arr_Size*sizeof(int), IPC_CREAT | 0666);
    int *sharedMem = shmat(shmid, (void *)0, 0);
    if (sharedMem == (void *)-1)
    {
        perror("Error in attach ");
        exit(-1);
    }
    int prevClk = getClk();
    remainingtime=sharedMem[id];
    while (remainingtime > 0) ////////////work to do here
    {
        if (prevClk != getClk())
        {
            remainingtime--; //no need for this variable
            sharedMem[id]--;
        }
        prevClk = getClk();
    }
    destroyClk(false);
    return 0;
}
