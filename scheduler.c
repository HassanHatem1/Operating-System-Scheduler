#include "headers.h"
#define msgq_key 65     // msgqueue for scheduler and process generator
#define sharedMemKey 44 // shared memory for remaining time of each process process
#define process_schedulerSHMKey 55
#define Total_MemorySize 1024
#define MAX_ProcessSize 256
void HPF();  // highest priority first
void SRTN(); // shortest remaining time next
void RR();   // Round Robin
void clearResources(int signum);

int process_count, quantum_time, msgq_id, sharedMemory_id, process_schedulerSHMID, arrivals_shm_id, finished_processes, totalrunning = 0;
double TWTA = 0, totalwaiting = 0;
int algorithm_num;
int *remainingTime;
int *weighted_TAs;
int *waiting_times;
int maxFinish = INT_MIN;

FILE *SchedulerLogFile;
FILE *SchedulerPerfFile;

// Create PCB from process
struct PCB *createPCB(Process proc)
{
    struct PCB *pcb = (struct PCB *)malloc(sizeof(struct PCB));
    pcb->id = proc.id;
    pcb->arrival = proc.arrival_time;
    pcb->burst = proc.running_time;
    pcb->priority = proc.priority;
    pcb->remainingTime = proc.running_time;
    pcb->memsize = proc.memsize;
    pcb->running = 0;
    pcb->wait = 0;
    pcb->stop = 0;
    pcb->stopped = false;
    return pcb;
}

// intialize message queue needed for communication between process generator and scheduler
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

// Msg queue recieving function
Msgbuff receiveProcess()
{
    Msgbuff msg;
    int rec_val = msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT);
    if (rec_val == -1)
    {
        msg.mtype = -1; // indicator that no message received
        return msg;
    }
    remainingTime[msg.proc.id] = msg.proc.running_time;
    return msg;
}

// intialize shared memory needed for the scheduler
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

    // shared memory for storing previous clock time between process and scheduler
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

// open the log file
void OpenSchedulerLogFile()
{
    SchedulerLogFile = fopen("SchedulerLog", "w"); // Open the file in write mode

    if (SchedulerLogFile == NULL)
    {
        printf("Error opening SchedularLog  file.\n");
        return;
    }
}

// round to 2 decimal places
double round2dp(double num)
{
    return round(num * 100.0) / 100.0;
}

// open the performance file and write the statistics
void OpenSchedulerPerfFile()
{
    SchedulerPerfFile = fopen("SchedulerPerf", "w"); // Open the file in write mode
    if (SchedulerPerfFile == NULL)
    {
        printf("Error opening SchedularPerf  file.\n");
        return;
    }
    else
    {
        int elapsed_time = maxFinish - 1; // total elapsed time from time 1 till finish time of last process
        double AWTA = round2dp(TWTA / process_count);
        double AWT = round2dp(totalwaiting / process_count);
        double sum = 0;
        for (int i = 1; i <= process_count; i++)
        {
            sum += pow((double)(weighted_TAs[i] - AWTA), (double)2);
        }
        double STD = round2dp(pow((sum / process_count), 0.5));
        double Utilization = round2dp((totalrunning * 100.0) / elapsed_time);
        fprintf(SchedulerPerfFile, " CPU Utilization = %.2f%% \n Avg. WTA = %.2f \n Avg. Waiting = %.2f \n Std. WTA = %.2f",
                Utilization, AWTA, AWT, STD);
    }
}

// When a process starts, it will update its attributes and it will write in the log file
void startProcess(struct PCB *process)
{
    int start_time = getClk();
    process->start = start_time;

    process->wait = (start_time - process->arrival);
    process->remainingTime = process->burst;
    fprintf(SchedulerLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",
            start_time, process->id, process->arrival, process->burst, process->burst, process->wait);
}

// When a process finishes, it will update its attributes and it will write in the log file
void finishProcess(struct PCB *process)
{
    int finish_time = getClk();
    process->finish = finish_time;

    if (finish_time >= maxFinish)
        maxFinish = finish_time; // to calculate total elapsed time

    process->remainingTime = 0;
    process->running = process->burst;
    totalrunning += process->running; // for CPU utilization
    int TA = finish_time - process->arrival;
    double WTA = (TA * 1.0000) / process->burst;

    // saving data for last statistics
    weighted_TAs[process->id] = WTA;
    waiting_times[process->id] = process->wait;
    TWTA += WTA;
    totalwaiting += process->wait;

    printf("process %d finished\n", process->id);
    fprintf(SchedulerLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
            finish_time, process->id, process->arrival, process->burst, 0, process->wait, TA, WTA);
}

// When a process stops, it will update its attributes and it will write in the log file
void continueProcess(struct PCB *process)
{
    int continue_time = getClk();
    process->wait += (continue_time - process->stop);
    int diff = process->burst - process->running;
    process->remainingTime = (diff < 0) ? 0 : diff; // burst - running time  ;

    fprintf(SchedulerLogFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
            continue_time, process->id, process->arrival, process->burst, process->remainingTime, process->wait);

    int *prev = (int *)shmat(process_schedulerSHMID, (void *)0, 0); //
    *prev = getClk();
    shmdt(prev);
    printf("process %d continued\n", process->id);

    // send continue signal to the process
    kill(process->pid, SIGCONT);
}

void stopProcess(struct PCB *process)
{
    int last_stop_time = getClk();
    process->stop = last_stop_time;

    if (algorithm_num == 3)
    {
        process->running += quantum_time;
    }
    else
    {
        process->running += process->burst - remainingTime[process->id];
    }
    process->remainingTime = process->burst - process->running;

    fprintf(SchedulerLogFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
            last_stop_time, process->id, process->arrival, process->burst, process->remainingTime, process->wait);

    printf("process %d stopped\n", process->id);

    // send stop signal to the process
    kill(process->pid, SIGSTOP); // stop the process
}

void testkill(int signum)
{
    printf("process killed by something \n");
    raise(SIGINT);
}

int main(int argc, char *argv[])
{
    // argv[0] count of process ;
    // argv[1] chosen algorithm ;
    // argv[2] quantum time if RR ;
    initClk();
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

    if (algorithm_num == 1)
        HPF();
    else if (algorithm_num == 2)
        SRTN();
    else if (algorithm_num == 3)
        RR();

    printf("end of scheduler : \n");

    fclose(SchedulerLogFile); // closeSchedularLogFile
    OpenSchedulerPerfFile();  // openSchedularPerfFile
    fclose(SchedulerPerfFile);

    shmdt(remainingTime);

    raise(SIGINT); // to clean all resources and end
}

void HPF()
{
    printf("HPF Starts\n");

    // In HPF we need to sort the processes according to their priority, so we will use a priority queue
    Node *PriorityQueue = NULL;

    // intialize the remaining time shared memory
    remainingTime = (int *)shmat(sharedMemory_id, (void *)0, 0);

    if (remainingTime == (void *)-1)
    {
        printf("Error attaching shared memory segment\n");
        exit(-1);
    }

    struct PCB *current_process = NULL;
    while (finished_processes < process_count)
    {
        // at start of each loop we need to check if there is any process arrived at this time
        // here a do while is used as we need to fill the queue with all the processes that arrived at a single arrival time
        // in case of ties, according to our priority queue implementation and the way that processes are sent by order,
        // if there are two processes with the same priority, the one that pushed first will be executed first
        // or in other words, the one which is present earlier in the the table of processes will be executed first
        Msgbuff msg;
        do
        {
            msg = receiveProcess();
            if (msg.mtype != -1 && msg.proc.arrival_time == getClk())
            {
                printf("entered\n");
                struct PCB *processPCB = createPCB(msg.proc);
                push(&PriorityQueue, processPCB, processPCB->priority);
                printf("process %d pushed in the priority  queue\n", processPCB->id);
            }
        } while (msg.mtype != -1 && msg.proc.arrival_time == getClk());

        // if there is no process running and there is a process in the queue, we will run it and pop it from the priority queue till it finishes
        if (!isEmptyPQ(&PriorityQueue) && (current_process == NULL))
        {
            current_process = peek(&PriorityQueue);
            pop(&PriorityQueue);

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

                execl("./process.out", id_str, count_str, NULL);
            }
            else // scheduler now will assign the PID to the process and start it
            {
                current_process->pid = PID;
                startProcess(current_process);
            }
        }

        // if there is a process running and its remaining time is = 0, we will finish it and set the current process to NULL and increment the finished processes counter
        if (current_process != NULL && remainingTime[current_process->id] == 0)
        {
            finishProcess(current_process);
            current_process = NULL;
            finished_processes++;
        }
    }
}

// this function is used to enqueue all the processes that arrived at a single clk time
void enque_processes(struct Queue *queue)
{
    Msgbuff msg;
    do
    {
        msg = receiveProcess();
        if (msg.mtype != -1 && msg.proc.arrival_time == getClk())
        {
            struct PCB *processPCB = createPCB(msg.proc);
            printf("process %d pushed in the queue at time %d\n", processPCB->id, getClk());
            enqueue(queue, processPCB);
        }
    } while (msg.mtype != -1 && msg.proc.arrival_time == getClk());
}

void RR()
{
    printf("Round Robin Starts\n");
    printf("Quantum time = %d\n", quantum_time);

    // In RR we only need a queue to store the processes
    struct Queue *readyQueue = createQueue(process_count);

    // intialize the remaining time shared memory
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
        // if there is a tie between two processes, the one that pushed in first will be executed first, which is also the one that is present earlier in the table of processes
        // if there is no process running and there is a process in the queue, we will run it and deque it from the queue
        if (!isEmpty(readyQueue) && !isRunningProcess)
        {
            current_running_process = dequeue(readyQueue);
            startTime = getClk();
            isRunningProcess = 1;

            // if it the first time the process runs, we will create it and start it
            if (current_running_process->running == 0)
            {
                int PID = fork();
                if (PID == -1)
                {
                    perror("Error in creating the process in scheduler RR \n");
                    exit(-1);
                }
                else if (PID == 0) // process
                {
                    char id_str[10];
                    char count_str[10];

                    sprintf(id_str, "%d", current_running_process->id);
                    sprintf(count_str, "%d", process_count);

                    execl("./process.out", id_str, count_str, NULL);
                }
                else // scheduler now will assign the PID to the process and start it
                {

                    current_running_process->pid = PID;
                    startProcess(current_running_process);
                }
            }
            else // means that the process was stopped before and now we need to continue it
            {
                continueProcess(current_running_process);
            }
        }

        // if there is a process running and its quantum is finished or even before the quantum is finished its remaining time became = 0
        // we will finish it and set the current process to NULL and increment the finished processes counter
        if (isRunningProcess && (((getClk() - startTime) == quantum_time) || (remainingTime[current_running_process->id] == 0)))
        {

            isRunningProcess = 0;
            startTime = 0;
            printf("----------remTime: %d, -----id %d\n", remainingTime[current_running_process->id], current_running_process->id);

            // if remtime is 0, then the process is finished
            if (remainingTime[current_running_process->id] == 0)
            {
                finished_processes++;
                finishProcess(current_running_process);
                current_running_process = NULL;
            }

            // if remtime is not 0, then the process is not finished and we need to stop it and enqueue it again
            // but before we enque it again, we need to enque all the other processes that arrived at this time if there is any
            else
            {

                // Check if the queue is empty and if the process has not finished its execution
                // If true, then do not stop the process and let it continue to run
                if (!isEmpty(readyQueue))
                {
                    stopProcess(current_running_process);
                    struct PCB *temp = current_running_process;
                    enque_processes(readyQueue);
                    enqueue(readyQueue, temp);
                    current_running_process = NULL;
                }
                else
                {
                    isRunningProcess = 1;
                    startTime = getClk();
                }
            }
        }

        // if there is no process running and there is no process in the queue, we will enque all the processes that arrived at this time
        enque_processes(readyQueue);
    }
}

void SRTN()
{
    printf("SRTN starts\n");

    int PID = 1;
    Msgbuff msg;

    // In SRTN we need to sort the processes according to their remaining time, so we will use a priority queue
    Node *PriorityQueue = NULL;

    struct PCB *current_process = NULL;

    // intialize the remaining time shared memory
    remainingTime = (int *)shmat(sharedMemory_id, (void *)0, 0);

    if (remainingTime == (void *)-1)
    {
        printf("Error attaching shared memory segment\n");
        exit(-1);
    }

    while (finished_processes < process_count)
    {
        // at ties the process that was pushed first will be executed first, which is also the one that is present earlier in the table of processes
        // at start of each loop we need to check if there is any process arrived at this time to push them
        do
        {
            msg = receiveProcess();
            if (msg.mtype != -1 && msg.proc.arrival_time == getClk())
            {
                struct PCB *processPCB = createPCB(msg.proc);
                printf("process %d pushed in the queue at %d\n", processPCB->id, getClk());
                push(&PriorityQueue, processPCB, processPCB->remainingTime);
            }
        } while (msg.mtype != -1 && msg.proc.arrival_time == getClk());

        // if there is no process running and there is a process in the queue, we will run it and pop it from the priority queue
        if (!isEmptyPQ(&PriorityQueue) && current_process == NULL)
        {
            printf("entered 1st at: %d\n", getClk());

            current_process = peek(&PriorityQueue);
            pop(&PriorityQueue);

            // if the process was stopped before, we will continue it
            if (current_process->stopped)
            {
                continueProcess(current_process);
            }

            // if it the first time the process runs, we will create it and start it
            else
            {
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

                    execl("./process.out", id_str, count_str, NULL);
                }
                else // scheduler now will assign the PID to the process and start it
                {
                    current_process->pid = PID;
                    startProcess(current_process);
                }
            }
        }

        // if there is a process running and its remaining time is = 0, we will finish it and set the current process to NULL and increment the finished processes counter
        if (current_process != NULL && remainingTime[current_process->id] == 0 && PID != 0)
        {
            printf("entered 2nd at: %d\n", getClk());
            finishProcess(current_process);
            current_process = NULL;
            finished_processes++;
        }

        // if there is a process running and there is a process in the queue with less remaining time, we will stop the current process and push it in the priority queue
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
    printf("Clearing due to interruption\n");
    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    shmctl(sharedMemory_id, IPC_RMID, NULL);
    shmctl(process_schedulerSHMID, IPC_RMID, NULL);
    raise(SIGKILL);
}