//Author: Ben Vuong
//Date: 3/4/2023
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>

#define MSKEY 789
#define KEY 123
#define SIZE 100

struct Clock
{
  int seconds;
  int nanoSeconds;
};

struct my_msgbuf
{
        long mtype;
        int data[2]; //data[0] = sec data[1] = nano
};

int main(int argc, char *argv[])
{
        int itr = argc;
        int termSecond;
        int termNano;
        int shm_id;
        struct Clock *systemClock;

        int msqid;
        struct my_msgbuf message;
        key_t key =ftok("msgq_file.txt", 'B');


        msqid = msgget(key, 0666 | IPC_CREAT);


        if(msgrcv(msqid, &message, sizeof(message.data),0,0)==-1)
        {
                perror("msgrcv");
                exit(1);
        }

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


        termSecond = systemClock->seconds + message.data[0];//calc the time that                      the process will terminate
        termNano = systemClock->nanoSeconds + message.data[1];
        if(termNano >= 1000000)
        {
                termSecond++;
                termNano=0;
        }

        int currentSecond = systemClock->seconds;
        int startSecond = systemClock->seconds;
        int currentNano = systemClock->nanoSeconds;
        printf("Worker PID:%d PPID:%d SysClockSecond:%d SysClockNano:%d TermSeco                     nd:%d TermNano:%d\n", getpid(), getppid(), currentSecond, currentNano, termSecon                     d, termNano);
        printf("--Just Starting %d and %d\n ",message.data[0], message.data[1]);
        int secondsPassed;
        while(termSecond >= systemClock->seconds|| termNano >= systemClock->nano                     Seconds)
        {
                if (currentSecond < systemClock->seconds)//periodically check sy                     stem clock
                {
                        printf("Worker PID:%d PPID:%d SysClockSecond:%d SysClock                     Nano:%d TermSecond:%d TermNano:%d\n", getpid(), getppid(), systemClock->seconds,                      systemClock->nanoSeconds,  termSecond, termNano);
                        secondsPassed = systemClock->seconds - startSecond;
                        printf("--%d seconds have passed since starting\n", seco                     ndsPassed);
                        currentSecond++;
                }
        }
        printf("Worker PID:%d PPID:%d SysClockSecond:%d SysClockNano:%d TermSeco                     nd:%d TermNano:%d\n", getpid(), getppid(), systemClock->seconds, systemClock->na                     noSeconds, termSecond, termNano);
        printf("--Terminating\n");
        if(shmdt(systemClock) == -1)
        {
                perror("shmdt");
                exit(1);
        }

        return 0;



}
