#include "headers.h"
#define msgq_key 65 // msgqueue for scheduler and process generator

void HPF();  // highest priority first
void SRTN(); // shortest remaining time next
void RR();   // Round Robin

int process_count, quantum_time, msgq_id;
// msgq_id --> msgqueue between scheduler and process generator

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
}

void intiliazeMessageQueue()
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
}

int main(int argc, char *argv[])
{
    initClk();
    // argv[0] count of process ;
    // argv[1] chosen algorithm ;
    // argv[2] quantum time if RR ;

    process_count = atoi(argv[0]);
    int algorithm_num = atoi(argv[1]);
    quantum_time = 0;
    if (algorithm_num == 3)
        quantum_time = atoi(argv[2]);

    printf("I am scheduler and I received these parameters \n");
    printf("%d %d %d \n", process_count, algorithm_num, quantum_time);

    // create the message queue ;
    intiliazeMessageQueue(); // intialize msgQueue between scheduler and process generator
    int recievedProcessesCnt = 0;
    while (recievedProcessesCnt < process_count)
    {
        Msgbuff recievedProcess;
        int rec_val = msgrcv(msgq_id, &recievedProcess, sizeof(recievedProcess.proc), 0, !IPC_NOWAIT);
        if (rec_val == -1)
        {
            perror("Error in recieving process from process generator\n");
            exit(-1);
        }

        printf("recieved process number : %d  at clk cylce : %d from process generator. with follwing data {%d, %d ,%d ,%d }\n",
               recievedProcessesCnt, getClk(), recievedProcess.proc.id, recievedProcess.proc.arrival_time, recievedProcess.proc.running_time, recievedProcess.proc.priority);

        recievedProcessesCnt++;
    }

    // TODO implement the scheduler :)
    if (algorithm_num == 1)
        HPF();
    else if (algorithm_num == 2)
        SRTN();
    else if (algorithm_num == 3)
        RR();

    // upon termination release the clock resources.
    // destroyClk(true);
}

void RR() // round robin
{
    printf("round Robin Starts\n");
    printf("Quantum time = %d\n", quantum_time);
    struct Queue *readyQueue = createQueue(process_count);
}
void SRTN()
{
}
void HPF()
{
}