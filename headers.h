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
#include <stdbool.h>

// typedef short bool;

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

// ---------------------------Queue implementation for RR => src "Geeks for Geeks" --------------------------------
struct Queue
{
    int front, rear, size;
    unsigned capacity;
    PCB *array;
};

// function to create a queue of given capacity.
// It initializes size of queue as 0
struct Queue * createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
 
    queue->rear = capacity - 1;
    queue->array = (PCB*)malloc(queue->capacity * sizeof(PCB));
    return queue;
}

// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
    return (queue->size == queue->capacity);
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, PCB item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    // printf("%d enqueued to queue\n", item);
}

// Function to remove an item from queue.
// It changes front and size
PCB dequeue(struct Queue * queue)
{
    if (isEmpty(queue))
    {
        PCB pcb;
        setPCB(&pcb, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        return pcb;
        
    }
    PCB item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
PCB front(struct Queue* queue)
{
    if (isEmpty(queue))
    {
        PCB pcb;
        setPCB(&pcb, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        return pcb;
        
    }
    return queue->array[queue->front];
}
 
// Function to get rear of queue
PCB rear(struct Queue* queue)
{
    if (isEmpty(queue))
    {
        PCB pcb;
        setPCB(&pcb, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        return pcb;
        
    }
    return queue->array[queue->rear];
}

