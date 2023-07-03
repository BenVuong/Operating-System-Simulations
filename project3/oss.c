//Author: Ben Vuong
//Date: 3/4/2023
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>

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

struct my_mgsbuf
{
        long mtype;
        int data[2];// data[0] = sec data [1] = nano
};

int main(int argc, char *argv[])
{
        int opt;
        int requiredArg1 = 0;
        int requiredArg2 = 0;
        int requiredArg3 = 0;
        int requiredArg4 = 0;

        FILE *file;

        char *number;
        char *maxSim;
        char *timeLimit;

        struct PCB processTable[20];
        int sim = 0;
        int processCount = 0;

        while ((opt = getopt(argc, argv, "hn:s:t:f")) !=-1)
        {
                switch (opt)
                {
                        case 'h':
                                printf("Help option selected. \n");
                                printf("use the -n argument to launch that number of processes\n");
                                printf("use the -s argument to limit how many process can run simultaneously\n");
                                printf("use the -t arugment to give the time limit, will randomly choose a timelimit between 1 to the given argument for each child process\n");
                                printf("use -f to print outputs of oss to log.txt\n");
                                break;
                        case 'n':
                                number = optarg;
                                requiredArg1 = 1;
                                break;
                        case 's':
                                maxSim = optarg;
                                requiredArg2 = 1;
                                break;
                        case 't':
                                timeLimit = optarg;
                                requiredArg3 = 1;
                                break;
                        case 'f':
                                requiredArg4 = 1;
                        default:
                                break;
                }
        }

        if(requiredArg1 == 0 || requiredArg2 == 0 || requiredArg3 == 0 || requiredArg4 == 0)
        {
                printf("Error Must provide arguments : -n -s -t -f\n");
                exit(1);
        }


        if((file = fopen("log.txt", "r")) == NULL)
        {
                printf("Log file doesn't exist, creating log file...\n");
                file = fopen("log.txt", "w");
        }
        else
        {
                printf("Log file found\n");
                fclose(file);
                file = fopen("log.txt", "w");
        }


        int shmid;
        struct Clock *clock;
        system("touch msgq_file.txt");
        key_t key = ftok("msgq_file.txt", 'B');
        struct my_mgsbuf message;
        int msqid;
        msqid = msgget(key, 0666 | IPC_CREAT);
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
                processTable[x].pid =0;
                processTable[x].occupied = 0;
                processTable[x].startSeconds = 0;
                processTable[x].startNano = 0;

        }




        printf("launching\n\n");
        fprintf(file, "launching\n\n");
        clock->seconds = 0;//initalize clock to 0
        clock->nanoSeconds =0;
        int  numOfProcessFinished = 0;
        int numLaunched = 0;
        int status;
        while(clock->seconds<60||numOfProcessFinished < atoi(number))
        {

                clock->nanoSeconds++;//increment clock
                if (clock->nanoSeconds >= 1000000)
                {
                        clock->seconds++;
                        clock->nanoSeconds = 0;
                }
                if (clock->nanoSeconds == 500000 || clock->nanoSeconds == 0)
                {
                        printf("\nOSS PID: %d SystemClockSeconds: %d, SystemClockNanoseconds: %d\n",getpid() ,clock->seconds, clock->nanoSeconds);//need to output PCB too
                        fprintf(file,"\nOSS PID: %d SystemClockSeconds: %d, SystemClockNanoseconds: %d\n",getpid() ,clock->seconds, clock->nanoSeconds);
                        printf("Process Table:\n");
                        fprintf(file, "Process Table:\n");
                        printf("Entry\tOccupied\tPID\tStartS\tStartN\n");
                        fprintf(file, "Entry\tOccupied\tPID\tStartS\tStartN\n");
                        for (int a = 0; a < atoi(number); a++)
                        {
                                printf("%d\t%d\t\t%d\t%d\t%d\n",a, processTable[a].occupied, processTable[a].pid, processTable[a].startSeconds, processTable[a].startNano);
                                fprintf(file,"%d\t%d\t\t%d\t%d\t%d\n",a, processTable[a].occupied, processTable[a].pid, processTable[a].startSeconds, processTable[a].startNano);
                        }
                        printf("\n");
                        fprintf(file,"\n");
                        if(numOfProcessFinished == atoi(number))
                        {
                                if(msgctl(msqid, IPC_RMID, NULL)==-1 )
                                {
                                        perror("msgctl");
                                        exit(1);
                                }
                                system("rm msgq_file.txt");
                                fclose(file);
                                exit(0);
                        }


                }
                int pid = waitpid(-1,&status,WNOHANG);
                if (pid == 0 || pid == -1)//Check to see if no child process has terminated
                {


                        if(processCount < atoi(number) && sim < atoi(maxSim))
                        {
                                processTable[processCount].startSeconds = clock->seconds;
                                processTable[processCount].startNano = clock->nanoSeconds;
                                processTable[processCount].occupied = 1;
                                processTable[processCount].pid = fork();
                                processCount++;
                                sim++;


                                //fork
                                if(processTable[processCount-1].pid==0)//fork
                                {
                                        srand(getpid());

                                        int secLimit = (rand() % atoi(timeLimit))+1;
                                        int nanoLimit = (rand()%1000000)+1;
                                        message.mtype =1;
                                        message.data[0] = secLimit;
                                        message.data[1]= nanoLimit;
                                        if(msgsnd(msqid, &message, sizeof(message.data),0) == -1)
                                        {
                                                perror("mgsnd");
                                                exit(1);
                                        }

                                        char secLimitStr [1000000];
                                        char nanoLimitStr [1000000];
                                        sprintf(secLimitStr, "%d", secLimit);
                                        sprintf(nanoLimitStr, "%d", secLimit);
                                        char *args[] = {"./worker", NULL};
                                        execvp(args[0], args);
                                        printf("error");
                                        fprintf(file,"error");
                                }


                        }

                }
                else
                {
                        //for loop to find proccess with pid
                        //once found get the index and use that to change occupied from 1 back to 0
                        printf("pid:%d terminated\n",pid);
                        fprintf(file, "pid:%d terminated\n",pid);
                        sim--;
                        numOfProcessFinished++;
                        printf("%d processes finished\n", numOfProcessFinished);
                        fprintf(file, "%d processes finished\n", numOfProcessFinished);
                        for(int y = 0; y < atoi(number); y++)
                        {
                                if (pid == processTable[y].pid)
                                {
                                        processTable[y].occupied = 0;
                                }
                        }
                }


        }
        shmdt(clock);
}
