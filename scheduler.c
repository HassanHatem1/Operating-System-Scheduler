#include "headers.h"
#define msgq_key 65     // msgqueue for scheduler and process generator
#define sharedMemKey 44 // shared memory for remaining time of each process process
#define process_schedulerSHMKey 55
// #define arrivals_shm_key 88 // shared memory for storing number of arrivals at every arrival time

void HPF();  // highest priority first
void SRTN(); // shortest remaining time next
void RR();   // Round Robin
void clearResources(int signum);

int process_count, quantum_time, msgq_id, sharedMemory_id, process_schedulerSHMID, arrivals_shm_id, finished_processes = 0;
int algorithm_num;
int *remainingTime;
int *weighted_TAs;
int *waiting_times;

// msgq_id --> msgqueue between scheduler and process generator

FILE *SchedulerLogFile;

struct PCB *createPCB(Process proc)
{
    struct PCB *pcb = (struct PCB *)malloc(sizeof(struct PCB));
    pcb->id = proc.id;
    pcb->arrival = proc.arrival_time;
    pcb->burst = proc.running_time;
    pcb->priority = proc.priority;
    pcb->remainingTime = proc.running_time; // burst - running time  ;
    pcb->running = 0;
    pcb->wait = 0;
    pcb->stop = 0;
    pcb->stopped = false;

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
    remainingTime[msg.proc.id] = msg.proc.running_time;
    return msg;
}
void intializeSharedMemory()
{
    // shared memory for storing remaining time between process and scheduler
    key_t key = ftok("SharedMemoryKeyFile", sharedMemKey);
    if (key == -1)
    {
        perror("Error in creating the key of Shared memory in scheduler\n");
        exit(-1);
    }
    sharedMemory_id = shmget(key, (process_count + 1) * sizeof(int), IPC_CREAT | 0666); // +1 working with 1-based indexing not 0-based indexing
    if (sharedMemory_id == -1)
    {
        perror("Error in creating the ID of shared memory in scheduler\n");
        exit(-1);
    }

    key_t key2 = ftok("process_schedulerSHM", process_schedulerSHMKey);
    if (key2 == -1)
    {
        perror("Error in creating the key of Shared memory in scheduler\n");
        exit(-1);
    }
    process_schedulerSHMID = shmget(key2, 1 * sizeof(int), IPC_CREAT | 0666);
    if (process_schedulerSHMID == -1)
    {
        perror("Error in creating the ID of between  process and scheduler \n");
        exit(-1);
    }

    printf("shared memory created successfully in scheduler with ID : %d \n", sharedMemory_id);
    printf("shared memory (for prevClk trial ) created successfully in scheduler with ID : %d \n", process_schedulerSHMID);
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
    process->wait = (start_time - process->arrival);
    process->remainingTime = process->burst;
    fprintf(SchedulerLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",
            start_time, process->id, process->arrival, process->burst, process->burst, process->wait);
}
void finishProcess(struct PCB *process)
{
    int finish_time = getClk();
    process->finish = finish_time;
    process->remainingTime = 0;

    if (algorithm_num == 1)
        process->running = process->burst;

    if (algorithm_num == 2)
        process->running += (process->burst - process->remainingTime);

    if (algorithm_num == 3)
        process->running += quantum_time;

    int TA = finish_time - process->arrival;
    double WTA = (TA * 1.0000) / process->burst;
    // saving data for last statistics
    weighted_TAs[process->id] = WTA;
    waiting_times[process->id] = process->wait;
    printf("process %d finished\n", process->id);
    fprintf(SchedulerLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
            finish_time, process->id, process->arrival, process->burst, 0, process->wait, TA, WTA);
}
void continueProcess(struct PCB *process)
{
    int continue_time = getClk();
    process->wait += (continue_time - process->stop);
    int diff = process->burst - process->running;
    process->remainingTime = (diff < 0) ? 0 : diff; // burst - running time  ;

    fprintf(SchedulerLogFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival, process->burst, process->remainingTime, process->wait);

    int *prev = (int *)shmat(process_schedulerSHMID, (void *)0, 0); //
    *prev = getClk();
    shmdt(prev);
    printf("process %d continued\n", process->id);

    kill(process->pid, SIGCONT);
}
void stopProcess(struct PCB *process)
{
    int last_stop_time = getClk();
    process->stop = last_stop_time;

    if (algorithm_num == 3)
        process->running += quantum_time;
    else
        process->running += process->burst - remainingTime[process->id];

    process->remainingTime = process->burst - process->running; // sharedmem[id]

    fprintf(SchedulerLogFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival, process->burst, process->remainingTime, process->wait);

    printf("process %d stopped\n", process->id);

    kill(process->pid, SIGSTOP); // stop the process
}

void testkill(int signum)
{
    printf("process killed by something \n");
    raise(SIGINT);
}
int main(int argc, char *argv[])
{
    initClk();
    // argv[0] count of process ;
    // argv[1] chosen algorithm ;
    // argv[2] quantum time if RR ;
    signal(SIGINT, clearResources);
    signal(SIGSTOP, testkill); // to test kill
    process_count = atoi(argv[0]);
    algorithm_num = atoi(argv[1]);
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

    // TODO implement the scheduler
    if (algorithm_num == 1)
        HPF();
    else if (algorithm_num == 2)
        SRTN();
    else if (algorithm_num == 3)
        RR();

    printf("end of scheduler : \n");

    fclose(SchedulerLogFile); // closeSchedularLogFile
    shmdt(remainingTime);
    // upon termination release the clock resources.

    raise(SIGINT); // to clean all resources and end
}
void HPF()
{
    printf("HPF Starts\n");
    // priority Queue ;
    Node *PriorityQueue = NULL; // head
    remainingTime = (int *)shmat(sharedMemory_id, (void *)0, 0);
    if (remainingTime == (void *)-1)
    {
        printf("Error attaching shared memory segment\n");
        exit(-1);
    }
    struct PCB *current_process = NULL;

    while (finished_processes < process_count)
    {
        Msgbuff msg;
        // here we fill the queue with the processes that arrived at a single arrival time
        // this ensure that if many processes arrived at the same time, the one with the highest priority will be executed first
        do
        {
            msg = receiveProcess();
            if (msg.mtype != -1)
            {
                printf("entered\n");
                struct PCB *processPCB = createPCB(msg.proc);
                push(&PriorityQueue, processPCB, processPCB->priority);
                printf("process %d pushed in the priority  queue\n", processPCB->id);
            }
        } while (msg.mtype != -1);

        if (!isEmptyPQ(&PriorityQueue) && (current_process == NULL))
        {
            current_process = peek(&PriorityQueue);
            pop(&PriorityQueue);
            startProcess(current_process);
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

                sprintf(id_str, "%d", current_process->id);
                sprintf(count_str, "%d", process_count);

                system("gcc -Wall -o process.out process.c -lm -fno-stack-protector");

                execl("./process.out", id_str, count_str, NULL);
            }
            else
            {
                current_process->pid = PID;
            }
        }
        if (current_process != NULL && remainingTime[current_process->id] == 0)
        {
            finishProcess(current_process);
            current_process = NULL;
            finished_processes++;
        }
    }
}
void RR() // round robin
{
    printf("Round Robin Starts\n");
    printf("Quantum time = %d\n", quantum_time);

    struct Queue *readyQueue = createQueue(process_count);
    remainingTime = (int *)shmat(sharedMemory_id, (void *)0, 0);
    if (remainingTime == (void *)-1)
    {
        printf("Error attaching shared memory segment in RR\n");
        exit(-1);
    }
    struct PCB *current_running_process = NULL;
    int startTime = 0, isRunningProcess = 0;
    while (finished_processes < process_count)
    {
        if (!isEmpty(readyQueue) && !isRunningProcess)
        {
            current_running_process = dequeue(readyQueue);
            startTime = getClk();
            isRunningProcess = 1;
            if (current_running_process->running == 0)
            {

                int PID = fork();
                if (PID == -1)
                {
                    perror("Error in creating the process in scheduler RR \n");
                    exit(-1);
                }
                else if (PID == 0)
                {
                    char id_str[10];
                    char count_str[10];

                    sprintf(id_str, "%d", current_running_process->id);
                    sprintf(count_str, "%d", process_count);

                    // system("gcc -Wall -o process.out process.c -lm -fno-stack-protector");

                    execl("./process.out", id_str, count_str, NULL);
                }
                else
                {

                    current_running_process->pid = PID;
                    startProcess(current_running_process);
                }
            }
            else
            { // means that the process was stopped and now we need to continue it
                continueProcess(current_running_process);
            }
        }

        if (isRunningProcess && (((getClk() - startTime) == quantum_time) || (remainingTime[current_running_process->id] == 0)))
        {
            isRunningProcess = 0;
            startTime = 0;
            printf("----------remTime: %d, -----id %d\n", remainingTime[current_running_process->id], current_running_process->id);

            if (remainingTime[current_running_process->id] == 0)
            {
                finished_processes++;
                finishProcess(current_running_process);
                current_running_process = NULL;
            }
            else
            {
                stopProcess(current_running_process);
                struct PCB *temp = current_running_process;
                // check if another process arrived at the same time and push it in the queue
                Msgbuff msg2;
                do
                {
                    msg2 = receiveProcess();
                    if (msg2.mtype != -1)
                    {
                        struct PCB *processPCB = createPCB(msg2.proc);
                        printf("process %d pushed in the queue at time %d\n", processPCB->id, getClk());
                        enqueue(readyQueue, processPCB);
                    }
                } while (msg2.mtype != -1);

                enqueue(readyQueue, temp);
                current_running_process = NULL;
            }
        }
        Msgbuff msg = receiveProcess();
        if (msg.mtype != -1)
        {
            struct PCB *processPCB = createPCB(msg.proc);
            printf("process %d pushed in the queue at time %d\n", processPCB->id, getClk());
            enqueue(readyQueue, processPCB);
        }
    }
}
void SRTN()
{
    printf("SRTN starts\n");
    int PID = 1;
    Msgbuff msg;
    Node *PriorityQueue = NULL;
    struct PCB *current_process = NULL;
    remainingTime = (int *)shmat(sharedMemory_id, (void *)0, 0);
    if (remainingTime == (void *)-1)
    {
        printf("Error attaching shared memory segment\n");
        exit(-1);
    }

    while (finished_processes < process_count)
    {
        do
        {
            msg = receiveProcess();
            if (msg.mtype != -1)
            {
                struct PCB *processPCB = createPCB(msg.proc);
                printf("process %d pushed in the queue at %d\n", processPCB->id, getClk());
                push(&PriorityQueue, processPCB, processPCB->remainingTime);
            }
        } while (msg.mtype != -1);

        if (!isEmptyPQ(&PriorityQueue) && current_process == NULL)
        {
            printf("entered 1st at: %d\n", getClk());

            current_process = peek(&PriorityQueue);
            pop(&PriorityQueue);

            if (current_process->stopped)
            {
                continueProcess(current_process);
            }
            else
            {
                startProcess(current_process);
                PID = fork();
                if (PID == -1)
                {
                    perror("Error in creating the process in scheduler SRTN \n");
                    exit(-1);
                }
                else if (PID == 0)
                {
                    char id_str[10];
                    char count_str[10];

                    sprintf(id_str, "%d", current_process->id);
                    sprintf(count_str, "%d", process_count);

                    system("gcc -Wall -o process.out process.c -lm -fno-stack-protector");

                    execl("./process.out", id_str, count_str, NULL);
                }
                else
                {
                    current_process->pid = PID;
                }
            }
        }


        if (current_process != NULL && remainingTime[current_process->id] == 0 && PID != 0)
        {
            printf("entered 2nd at: %d\n", getClk());
            finishProcess(current_process);
            current_process = NULL;
            finished_processes++;
        }
        else if (current_process != NULL && !isEmptyPQ(&PriorityQueue) && remainingTime[current_process->id] > peek(&PriorityQueue)->remainingTime && PID != 0)
        {
            printf("entered 3rd at: %d\n", getClk());
            stopProcess(current_process);
            current_process->stopped = true;
            push(&PriorityQueue, current_process, remainingTime[current_process->id]);
            current_process = NULL;
        }
    }
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    printf("Clearing due to interruption\n");
    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    shmctl(sharedMemory_id, IPC_RMID, NULL);
    shmctl(process_schedulerSHMID, IPC_RMID, NULL);
    raise(SIGKILL);
}