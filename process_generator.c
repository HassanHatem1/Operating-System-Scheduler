#include "headers.h"
#include <stdio.h>

typedef struct
{
    int id;
    int priority;
    int arrival_time;
    int running_time;
} Process;

void clearResources(int);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    FILE *file = fopen("processes.txt", "r");
    if (file == NULL)
    {
        printf("Error! File not found.\n");
        return 1;
    }

    // get the number of processes from the document by counting the lines
    int processes_count = 0;
    char ch;
    while (!feof(file))
    {
        ch = fgetc(file);
        if (ch == '\n')
            processes_count++;
    }
    fclose(file);

    // create an array of processes with the size of the number of processes
    Process processes[processes_count];

    // reopen the file to read the processes data from the document and store them in the array
    file = fopen("processes.txt", "r");
    for (int i = 0; i < processes_count; i++)
        // here t%d will make fscanf ignore the space and read the number only
        fscanf(file, "%d t%d t%d t%d", &processes[i].id, &processes[i].priority, &processes[i].arrival_time, &processes[i].running_time);
    fclose(file);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int algorithm_num;
    printf("-----------------PROCESS_GENERATOR-----------------\n");
    printf("Choose a scheduling algorithm from the following:\n");
    printf("1. Non-preemptive Highest Priority First (HPF).\n");
    printf("2. Shortest Remaining time Next (SRTN).\n");
    printf("3. Round Robin (RR).\n");
    scanf("%d", &algorithm_num);
    while(algorithm_num < 1 || algorithm_num > 3)
    {
        printf("Please enter a valid algorithm number: ");
        scanf("%d", &algorithm_num);
    }
    printf("---------------------------------------------------\n");

    int quantum;
    if (algorithm_num == 3)
    {
        printf("Enter the quantum for the Round Robin algorithm: ");
        scanf("%d", &quantum);
    }

    // 3. Initiate and create the scheduler and clock processes.

    // Create the scheduler process
    pid_t scheduler_pid = fork();
    if (scheduler_pid == -1)
        perror("Error in creating scheduler process\n");
    else if (scheduler_pid == 0) // This is the scheduler process
    {
        // compile the C program named scheduler.c using the gcc compiler with certain flags ().
        // The compiled output is named scheduler.out.
        system("gcc -Wall -o scheduler.out scheduler.c -lm -fno-stack-protector");
        printf("Scheduling..\n");

        // execute the scheduler.out file with the arguments of the number of processes and the algorithm number (and the quantum if it's RR)
        // sprintf is used to convert the integer arguments to strings to be passed to the execl function
        char process_count_str[10], algorithm_num_str[2], quantum_str[10];
        sprintf(process_count_str, "%d", processes_count);
        sprintf(algorithm_num_str, "%d", algorithm_num);

        // here the NULL in the execl function serves as a sentinel value to indicate the end of the argument list.
        if (algorithm_num != 3)
            execl("./scheduler.out", process_count_str, algorithm_num_str, NULL);
        else
        {
            sprintf(quantum_str, "%d", quantum);
            execl("./scheduler.out", process_count_str, algorithm_num_str, quantum_str, NULL);
        }
        exit(0);
    }

    // Create the clock process
    pid_t clock_pid = fork();
    if (scheduler_pid == -1)
        perror("Error in creating scheduler process\n");
    else if (clock_pid == 0) // This is the clock process
    {
        // compile the C program named clk.c using the gcc compiler with certain flags().
        // The compiled output is named clk.out.
        system("gcc clk.c -o clk.out -fno-stack-protector");
        execl("./clk.out", NULL); // execute the clk.out file
        exit(0);
    }

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // 5. Create a data structure for processes and provide it with its parameters.
    // --> already done in the beginning of the main function

    // 6. Send the information to the scheduler at the appropriate time.
    int index = 1;
    while (index <= processes_count)
    {
        if (processes[index].arrival_time == getClk())
        {
           // send_val = msgsnd(msgq_id, &processes[current], sizeof(struct processData), !IPC_NOWAIT);
            //current++;
        }
    }

// 7. Clear clock resources
destroyClk(true);
}


void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    printf("Clearing due to interruption\n");
    // msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    raise(SIGKILL);
}