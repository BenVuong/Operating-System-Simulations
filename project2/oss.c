//Author: Ben Vuong
//Date: 2/19/2022
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define KEY 123
#define SIZE 100

struct PCB
{
        int occupied; // either ture or false
        pid_t pid; //process id of this child
        int startSeconds; //time when it was forked
        int startNano; //time when it was forked
};

struct Clock
{
        int seconds;
        int nanoSeconds;
};

int main(int argc, char *argv[])
{
        srand(time(NULL));
        int opt;
        int requiredArg1 = 0;
        int requiredArg2 = 0;
        int requiredArg3 = 0;

        char *number;
        char *maxSim;
        char *timeLimit;

        struct PCB processTable[20];
        int sim = 0;
        int processCount = 0;

        while ((opt = getopt(argc, argv, "hn:s:t:")) !=-1)
        {
                switch (opt)
                {
                        case 'h':
                                break;
                        case 'n':
                                number = optarg;
                                break;
                        case 's':
                                maxSim = optarg;
                                break;
                        case 't':
                                timeLimit = optarg;
                                break;
                        default:
                                break;
                }
        }

        int shmid;
        struct Clock *clock;

        shmid = shmget(KEY, SIZE, IPC_CREAT | 00666);
        if (shmid < 0)
        {
                perror("shmget");
                return 1;
        }

        clock = shmat(shmid, NULL, 0);
        if(clock == (struct Clock *)-1)
        {
                perror("shmat");
                return 1;
        }

        for(int x = 0; x < atoi(number); x++)
        {
                processTable[x].pid = getpid()+x+1;
                processTable[x].occupied = 0;
                processTable[x].startSeconds = 0;
                processTable[x].startNano = 0;

        }
        printf("launch pid:%d\n", processTable[0].pid);
//      printf("Number:%d and MaxSim:%d\n", atoi(number), atoi(maxSim));
        clock->seconds = 0;//initalize clock to 0
        clock->nanoSeconds =0;
        int  numOfProcessFinished = 0;
        int numLaunched = 0;
        int status;
        while(1)//numOfProcessFinished < atoi(number))
        {
                clock->nanoSeconds++;//increment clock
                if (clock->nanoSeconds >= 1000000000)
                {
                        clock->seconds++;
                        clock->nanoSeconds = 0;
                }
                if (clock->nanoSeconds == 500000000 || clock->nanoSeconds == 0)
                {
                        printf("OSS PID: %d SystemClockSeconds: %d, SystemClockNanoseconds: %d\n",getpid() ,clock->seconds, clock->nanoSeconds);//need to output PCB too
                        printf("Process Table:\n");
                        printf("Entry\tOccupied\tPID\tStartS\tStartN\n");
                        for (int a = 0; a < atoi(number); a++)
                        {
                                printf("%d\t%d\t\t%d\t%d\t%d\n",a, processTable[a].occupied, processTable[a].pid, processTable[a].startSeconds, processTable[a].startNano);
                        }
                }
                int pid = waitpid(-1,&status,WNOHANG);
                if (pid == 0)
                {
                        if(processCount < atoi(number))
                        {
                                processTable[processCount].startSeconds = clock->seconds;
                                processTable[processCount].startNano = clock->nanoSeconds;
                                processCount++;
                                //fork


                        }
                        else
                        {
                                numOfProcessFinished++;
                        }

                }



        }

        shmdt(clock);
}
