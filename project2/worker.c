//Author: Ben Vuong
//Date: 2/19/2023
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>

#define KEY 123
#define SIZE 100

struct Clock
{
  int seconds;
  int nanoSeconds;
};
int main(int argc, char *argv[])
{
        int itr = argc;
        char *a = argv[1];
        char *b =argv[2];
        int sec = atoi(a);
        int nano = atoi(b);
        int termSecond;
        int termNano;
        int shm_id;
        struct Clock *systemClock;

        shm_id = shmget(KEY, SIZE,0);
        if (shm_id < 0)
        {
                perror("shmget");
                exit(1);
        }

        systemClock = shmat(shm_id, NULL, 0);
        if(systemClock == (void *)-1)
        {
                perror("shmat");
                exit(1);
        }


        termSecond = systemClock->seconds + sec;//calc the time that the process will terminate
        termNano = systemClock->nanoSeconds + nano;
        if(termNano >= 1000000)
        {
                termSecond++;
                termNano=0;
        }

        int currentSecond = systemClock->seconds;
        int startSecond = systemClock->seconds;
        int currentNano = systemClock->nanoSeconds;
        printf("Worker PID:%d PPID:%d SysClockSecond:%d SysClockNano:%d TermSecond:%d TermNano:%d\n", getpid(), getppid(), currentSecond, currentNano, termSecond, termNano);
        printf("--Just Starting\n");
        int secondsPassed;
        while(termSecond >= systemClock->seconds|| termNano >= systemClock->nanoSeconds)
        {
                if (currentSecond < systemClock->seconds)//periodically check system clock
                {
                        printf("Worker PID:%d PPID:%d SysClockSecond:%d SysClockNano:%d TermSecond:%d TermNano:%d\n", getpid(), getppid(), systemClock->seconds, systemClock->nanoSeconds,  termSecond, termNano);
                        secondsPassed = systemClock->seconds - startSecond;
                        printf("--%d seconds have passed since starting\n", secondsPassed);
                        currentSecond++;
                }
        }
        printf("Worker PID:%d PPID:%d SysClockSecond:%d SysClockNano:%d TermSecond:%d TermNano:%d\n", getpid(), getppid(), systemClock->seconds, systemClock->nanoSeconds, termSecond, termNano);
        printf("--Terminating\n");
        if(shmdt(systemClock) == -1)
        {
                perror("shmdt");
                exit(1);
        }

        return 0;



}
