#include "headers.h"

int main(int argc, char *argv[])
{
    // printf("%d", getClk());
    // initClk();
    // argv[0] count of process ;
    // argv[1] chosen algorithm ;
    // argv[2] quantum time if RR ;
    int process_count = atoi(argv[0]);
    int algorithm_num = atoi(argv[1]);
    int quantum_time = 0;

    if (algorithm_num == 3)
        quantum_time = atoi(argv[2]);

    printf("I am scheduler and I received these parameters \n");
    printf("%d %d %d \n", process_count, algorithm_num, quantum_time);

    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    // destroyClk(true);
}
