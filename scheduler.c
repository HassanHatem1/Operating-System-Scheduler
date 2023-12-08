#include "headers.h"
#define msgq_key 65     // msgqueue for scheduler and process generator
#define sharedMemKey 44 // shared memory for remaining time of each process process

void HPF();  // highest priority first
void SRTN(); // shortest remaining time next
void RR();   // Round Robin
void clearResources(int signum);

int process_count, quantum_time, msgq_id, sharedMemory_id;
int *weighted_TAs;
int *waiting_times;

// msgq_id --> msgqueue between scheduler and process generator

FILE *SchedulerLogFile;

struct PCB *createPCB(Process proc)
{
    struct PCB *pcb = (struct PCB *)malloc(sizeof(struct PCB));
    pcb->id = proc.id;
    pcb->arrival = proc.arrival_time;
    pcb->brust = proc.running_time;
    pcb->priority = proc.priority;
    pcb->remaningTime = proc.running_time; // burst - running time  ;
    pcb->running = 0;
    pcb->wait = 0;

    return pcb;
}
Msgbuff receiveProcess()
{
    Msgbuff msg;
    int rec_val = msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT);
    if (rec_val == -1)
    {
        msg.mtype = -1; // indicator that no message received
        // printf("fuck it \n");
        return msg;
    }
    int *remaningTime = (int *)shmat(sharedMemory_id, (void *)0, 0);
    printf("process %d recieved\n", msg.proc.id);
    if (remaningTime == (void *)-1)
    {
        printf("Error attaching shared memory segment\n");
        raise(SIGINT);
    }
    remaningTime[msg.proc.id] = msg.proc.running_time;
    return msg;
}
void intializeSharedMemory()
{
    key_t key = ftok("SharedMemoryKeyFile", sharedMemKey); // for shared memory
    if (key == -1)
    {
        perror("Error in creating the key of Shared memory in  scheduler\n");
        exit(-1);
    }
    sharedMemory_id = shmget(key, (process_count + 1) * sizeof(int), IPC_CREAT | 0666); // +1 working with 1-based indexing not 0-based indexing
    if (sharedMemory_id == -1)
    {
        perror("Error in creating the ID of shared memory in scheduler\n");
        exit(-1);
    }

    printf("shared memory created successfully in scheduler with ID : %d \n", sharedMemory_id);
}
void intializeMessageQueue()
{
    key_t key_id;
    key_id = ftok("msgQueueFileKey", msgq_key);
    if (key_id == -1)
    {
        perror("Error in creating the key in  scheduler\n");
        exit(-1);
    }
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); // create the message queue
    if (msgq_id == -1)
    {
        perror("Error in creating the message queue in scheduler\n");
        exit(-1);
    }
    printf("Message queue created successfully in scheduler with ID : %d \n", msgq_id);
}

void OpenSchedulerLogFile()
{
    SchedulerLogFile = fopen("SchedulerLog", "w"); // Open the file in write mode

    if (SchedulerLogFile == NULL)
    {
        printf("Error opening SchedularLog  file.\n");
        return;
    }
}

void startProcess(struct PCB *process)
{
    int start_time = getClk();
    process->start = start_time;
    process->wait = start_time - process->arrival;
    fprintf(SchedulerLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",
            start_time, process->id, process->arrival, process->brust, process->brust, process->wait);
}
void finishProcess(struct PCB *process)
{
    int finish_time = getClk();
    process->finish = finish_time;
    process->remaningTime = 0;
    process->running = process->brust;
    int TA = finish_time - process->arrival;
    double WTA = (TA * 1.0000) / process->brust;
    // saving data for last statistics
    weighted_TAs[process->id] = WTA;
    waiting_times[process->id] = process->wait;

    fprintf(SchedulerLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
            finish_time, process->id, process->arrival, process->brust, 0, process->wait, TA, WTA);
}

int main(int argc, char *argv[])
{
    initClk();
    // argv[0] count of process ;
    // argv[1] chosen algorithm ;
    // argv[2] quantum time if RR ;
    signal(SIGINT, clearResources);

    process_count = atoi(argv[0]);
    int algorithm_num = atoi(argv[1]);
    quantum_time = 0;
    if (algorithm_num == 3)
        quantum_time = atoi(argv[2]);

    printf("I am scheduler and I received these parameters \n");
    printf("%d %d %d \n", process_count, algorithm_num, quantum_time);

    waiting_times = (int *)malloc((process_count + 1) * sizeof(int));
    weighted_TAs = (int *)malloc((process_count + 1) * sizeof(int));

    intializeSharedMemory(); // intialize shared memory for remaining time of each process process
    intializeMessageQueue(); // intialize msgQueue between scheduler and process generator
    OpenSchedulerLogFile();  // openSchedularLogFile

    printf("Scheduler starts\n");

    // TODO implement the scheduler :)
    if (algorithm_num == 1)
        HPF();
    else if (algorithm_num == 2)
        SRTN();
    else if (algorithm_num == 3)
        RR();

    printf("end of scheduler : \n");

    fclose(SchedulerLogFile); // closeSchedularLogFile
    // upon termination release the clock resources.

    raise(SIGINT); // to clean all resources and end
}
void HPF()
{
    int finishedprocess = 0;
    printf("HPF Starts\n");
    // priority Queue ;
    Node *PriorityQueue = NULL; // head
    int *remaningTime = (int *)shmat(sharedMemory_id, (void *)0, 0);
    struct PCB *Frontprocess = NULL;

    while (finishedprocess < process_count)
    {

        Msgbuff msg = receiveProcess();
        if (msg.mtype != -1)
        {
            printf("entered\n");
            struct PCB *processPCB = createPCB(msg.proc);
            // printf("process %d pushed in the queue\n", processPCB->id);
            push(&PriorityQueue, processPCB, processPCB->priority);
            printf("process %d pushed in the queue\n", processPCB->id);
        }
        if (!isEmptyPQ(&PriorityQueue) && (Frontprocess == NULL))
        {
            Frontprocess = peek(&PriorityQueue);
            pop(&PriorityQueue);
            startProcess(Frontprocess);
            int PID = fork();
            if (PID == -1)
            {
                perror("Error in creating the process in scheduler HPF \n");
                exit(-1);
            }
            else if (PID == 0)
            {
                char id_str[10];
                char count_str[10];

                sprintf(id_str, "%d", Frontprocess->id);
                sprintf(count_str, "%d", process_count);

                system("gcc -Wall -o process.out process.c -lm -fno-stack-protector");

                execl("./process.out", id_str, count_str, NULL);
            }
            else
            {
                Frontprocess->pid = PID;
            }
        }
        if (Frontprocess != NULL && remaningTime[Frontprocess->id] == 0)
        {
            finishProcess(Frontprocess);
            Frontprocess = NULL;
            finishedprocess++;
        }
    }
}
void RR() // round robin
{
    printf("Round Robin Starts\n");
    printf("Quantum time = %d\n", quantum_time);

    struct Queue *readyQueue = createQueue(process_count);
    int *remaningTime = (int *)shmat(sharedMemory_id, (void *)0, 0);
    struct PCB *CurrentRunningProcess = NULL;
    int finishedprocess = 0;

    while (finishedprocess < process_count)
    {
        Msgbuff msg = receiveProcess();
        if (msg.mtype != -1)
        {
            struct PCB *processPCB = createPCB(msg.proc);
            printf("process %d pushed in the queue\n", processPCB->id);
            enqueue(readyQueue, processPCB);
        }
        // if (!isEmpty(readyQueue))
    }
}
void SRTN()
{
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    printf("Clearing due to interruption\n");
    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    int result = shmctl(sharedMemory_id, IPC_RMID, NULL);
    raise(SIGKILL);
}