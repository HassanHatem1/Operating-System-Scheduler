#include "headers.h"

void HPF();  // highest priority first
void SRTN(); // shortest remaining time next
void RR();   // Round Robin

int process_count, quantum_time;

int main(int argc, char *argv[])
{
    initClk();
    printf("schedulaer me %d \n", getClk());
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

void RR()
{
}