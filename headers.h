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
#include <math.h>

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
    int burst;
    int finish;
    int running;
    int remainingTime;
    int stop;
    int priority;
    int start;
    int wait;
    bool stopped;
    int memsize;
    int startwait;
};

void setPCB(struct PCB *pcb, int nID, int nPID, int nArrival, int nBurst, int nFinish, int nRunning, int nStop, int nPriority, int nStart, int nWait, int nremainingTime)
{
    pcb->id = nID;
    pcb->pid = nPID;
    pcb->arrival = nArrival;
    pcb->burst = nBurst;
    pcb->finish = nFinish;
    pcb->running = nRunning;
    pcb->stop = nStop;
    pcb->remainingTime = nremainingTime;
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
    int memsize;
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
        printf("the node number %d which has id %d and burst time %d\n", index, queue->array[i]->id, queue->array[i]->burst);
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
        printf("the node number %d which has id %d and burst time %d\n  and priority %d \n", index, start->process->id, start->process->burst, start->priority);
        index++;
        start = start->next;
    }
}

typedef struct buddy_treeNode buddy_treeNode;
struct buddy_treeNode
{
    int process_id;
    buddy_treeNode *left;
    buddy_treeNode *right;
    buddy_treeNode *parent;
    int flag;
    int size;
    int begin;
    int end;
};

buddy_treeNode *createRoot()
{
    buddy_treeNode *root = (buddy_treeNode *)malloc(sizeof(buddy_treeNode));
    root->size = 1024;
    root->begin = 0;
    root->end = 1023;
    root->flag = 0;
    root->process_id = -1;
    root->left = NULL;
    root->right = NULL;
    root->parent = NULL;
    return root;
}

void WriteToMemoryLogFile(buddy_treeNode *root, bool isAllocation, int process_real_size);

buddy_treeNode *find_node(buddy_treeNode *root, int size)
{
    if (root == NULL || root->flag == 1)
    {
        return NULL;
    }
    if (root->size == size && root->left == NULL && root->right == NULL)
    {
        return root;
    }
    buddy_treeNode *left = find_node(root->left, size);
    if (left != NULL)
    {
        return left;
    }
    return find_node(root->right, size);
}

bool buddy_allocate(buddy_treeNode *root, int size, int process_id, int process_real_size)
{
    buddy_treeNode *node = find_node(root, size);
    if (node != NULL)
    {
        node->flag = 1;
        node->process_id = process_id;
        WriteToMemoryLogFile(node, true, process_real_size);
        return true;
    }
    if (root->size < size)
    {
        return false;
    }
    if (root->flag == 1)
    {
        return false;
    }
    if (root->left == NULL)
    {
        root->left = (buddy_treeNode *)malloc(sizeof(buddy_treeNode));
        root->left->size = root->size / 2;
        root->left->begin = root->begin;
        root->left->end = root->begin + root->left->size - 1;
        root->left->flag = 0;
        root->left->process_id = -1;
        root->left->left = NULL;
        root->left->right = NULL;
        root->left->parent = root;
    }
    if (root->right == NULL)
    {
        root->right = (buddy_treeNode *)malloc(sizeof(buddy_treeNode));
        root->right->size = root->size / 2;
        root->right->begin = root->begin + root->right->size;
        root->right->end = root->end;
        root->right->flag = 0;
        root->right->process_id = -1;
        root->right->left = NULL;
        root->right->right = NULL;
        root->right->parent = root;
    }
    if (buddy_allocate(root->left, size, process_id, process_real_size))
    {
        return true;
    }
    else
    {
        return buddy_allocate(root->right, size, process_id, process_real_size);
    }
}

bool buddy_deallocate(buddy_treeNode *root, int process_id, int process_real_size)
{
    if (root->process_id == process_id)
    {
        WriteToMemoryLogFile(root, false, process_real_size);
        root->flag = 0;
        root->process_id = -1;
        return true;
    }
    else
    {
        if (root->left == NULL)
        {
            return false;
        }

        bool deallocated = buddy_deallocate(root->left, process_id, process_real_size) || buddy_deallocate(root->right, process_id, process_real_size);

        if (root->left != NULL && root->right != NULL && root->left->flag == 0 && root->right->flag == 0 && root->left->left == NULL && root->right->right == NULL)
        {
            free(root->left);
            free(root->right);
            root->left = NULL;
            root->right = NULL;
        }

        return deallocated;
    }
}

FILE *MemoryLogFile;

void OpenMemoryLogFile()
{
    MemoryLogFile = fopen("Memory.log", "w"); // Open the file in write mode

    if (MemoryLogFile == NULL)
    {
        printf("Error opening MemoryLog  file.\n");
        return;
    }
    fprintf(MemoryLogFile, "#At time x allocated y bytes for process z from i to j\n");
}

void WriteToMemoryLogFile(buddy_treeNode *root, bool allocOrDealloc, int process_real_size)
{

    if (allocOrDealloc)
    {
        fprintf(MemoryLogFile, "At time %d allocated %d bytes for process %d from %d to %d\n",
                getClk(), process_real_size, root->process_id, root->begin, root->end);
    }
    else
    {
        fprintf(MemoryLogFile, "At time %d freed %d bytes for process %d from %d to %d\n",
                getClk(), process_real_size, root->process_id, root->begin, root->end);
    }
}

void buddy_print(buddy_treeNode *root)
{
    if (root->left == NULL)
    {
        printf("begin:%d end:%d size:%d flag:%d process_id:%d\n", root->begin, root->end, root->size, root->flag, root->process_id);
    }
    else
    {
        buddy_print(root->left);
        buddy_print(root->right);
    }
}
