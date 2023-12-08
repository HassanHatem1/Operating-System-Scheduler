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
struct PCB
{
    int id;
    int pid;
    int arrival;
    int brust;
    int finish;
    int running;
    int remaningTime;
    int stop;
    int priority;
    int start;
    int wait;
};

void setPCB(struct PCB *pcb, int nID, int nPID, int nArrival, int nBurst, int nFinish, int nRunning, int nStop, int nPriority, int nStart, int nWait, int nRemaningTime)
{
    pcb->id = nID;
    pcb->pid = nPID;
    pcb->arrival = nArrival;
    pcb->brust = nBurst;
    pcb->finish = nFinish;
    pcb->running = nRunning;
    pcb->stop = nStop;
    pcb->remaningTime = nRemaningTime;
    pcb->priority = nPriority;
    pcb->start = nStart;
    pcb->wait = nWait;
}
//-------------------------Process and Process messages structs  ----------------------------------------------------------------
typedef struct
{
    int id;
    int priority;
    int arrival_time;
    int running_time;
} Process;

typedef struct
{
    long mtype;
    Process proc;
} Msgbuff;
// ---------------------------Queue implementation for RR => src "Geeks for Geeks" --------------------------------
struct Queue
{
    int front, rear, size;
    unsigned capacity;
    struct PCB **array; // array of pointers
};

// function to create a queue of given capacity.
// It initializes size of queue as 0
struct Queue *createQueue(unsigned capacity)
{
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;

    queue->rear = capacity - 1;
    queue->array = (struct PCB **)malloc(queue->capacity * sizeof(struct PCB *));
    return queue;
}

// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue *queue)
{
    return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmpty(struct Queue *queue)
{
    return (queue->size == 0);
}

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue *queue, struct PCB *process)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = process;
    queue->size = queue->size + 1;
}

// Function to remove an item from queue.
// It changes front and size
struct PCB *dequeue(struct Queue *queue)
{
    if (isEmpty(queue))
    {
        return NULL;
    }
    struct PCB *process = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return process;
}
struct PCB *front(struct Queue *queue)
{
    if (isEmpty(queue))
    {
        return NULL;
    }
    return queue->array[queue->front];
}

// Function to get rear of queue
struct PCB *rear(struct Queue *queue)
{
    if (isEmpty(queue))
    {
        return NULL;
    }
    return queue->array[queue->rear];
}
void QueuePrint(struct Queue *queue)
{
    int index = 1;
    for (int i = queue->front; i <= queue->rear; i++)
    {
        printf("the node number %d which has id %d and burst time %d\n", index, queue->array[i]->id, queue->array[i]->brust);
        index++;
    }
}
// --------------------Priority Queue implementation for HPF /STRN => src "Geeks for Geeks" ----------------------------
typedef struct node
{
    struct PCB *process;

    // Lower values indicate higher priority
    int priority;

    struct node *next;

} Node;

// Function to Create A New Node
Node *newNode(struct PCB *d, int p)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->process = d;
    temp->priority = p;
    temp->next = NULL;

    return temp;
}

// Return the value at head
struct PCB *peek(Node **head)
{
    return (*head)->process;
}

// Removes the element with the
// highest priority from the list
void pop(Node **head)
{
    Node *temp = *head;
    (*head) = (*head)->next;
    free(temp);
}

// Function to push according to priority
void push(Node **head, struct PCB *d, int p)
{
    Node *start = (*head);

    if (start == NULL)
    {
        (*head) = newNode(d, p);
        return;
    }

    // Create new Node
    Node *temp = newNode(d, p);

    // Special Case: The head of list has lesser
    // priority than new node. So insert new
    // node before head node and change head node.
    if ((*head)->priority > p)
    {

        // Insert New Node before head
        temp->next = *head;
        (*head) = temp;
    }
    else
    {

        // Traverse the list and find a
        // position to insert new node
        while (start->next != NULL &&
               start->next->priority <= p)
        {
            start = start->next;
        }

        // Either at the ends of the list
        // or at required position
        temp->next = start->next;
        start->next = temp;
    }
}

// Function to check is list is empty
int isEmptyPQ(Node **head)
{
    return ((*head) == NULL);
}

void PQueuePrint(Node **head)
{
    Node *start = *head;
    int index = 1;
    while (start)
    {
        printf("the node number %d which has id %d and burst time %d\n  and priority %d \n", index, start->process->id, start->process->brust, start->priority);
        index++;
        start = start->next;
    }
}