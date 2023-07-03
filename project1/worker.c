//Author: Ben Vuong
//Date: 1/30/2023
//Will take in a integer argument which will be use to iterate through
//printing pid and ppid that number of times.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
        int itr = argc;
        char *a = argv[1];
        int num = atoi(a); //take in the number argument.
        for(int x = 0; x < num; x++) //number is use to loop through that number of iterations
        {
                int pid, ppid;
                pid = getpid();
                ppid = getppid();

                printf("Worker PID: %d", pid);
                printf(" Worker PPID: %d", ppid);
                printf(" Iteration: %d", x+1);
                printf(" Before Sleep\n");
                sleep(1);
                printf("Worker PID: %d", pid);
                printf(" Worker PPID: %d", ppid);
                printf(" Iteration: %d", x+1);
                printf(" After Sleep\n");
                printf("\n");
        }
        printf("complete\n");
        exit(0);
}
