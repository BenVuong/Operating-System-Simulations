//Author: Ben Vuong
//Date: 4/26/2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>


#define MSKEY 789
#define KEY 123
#define SIZE 100

struct Clock
{
        int sec;
        int nano;
};

struct my_msgbuf
{
        long mtype;
        int data;
        int pageNum;
        int address;
        pid_t senderPid;
};


void timeout_handler(int sig) {
        printf("\nTimeout occurred! Clearing out worker %d\n",getpid());
        exit(1);
}

int main(int argc, char *argv[])
{
        printf("Hello this is Worker\n");
        struct itimerval timer;//set up timeout. Terminate after 3 seconds
        signal(SIGALRM, timeout_handler);
        timer.it_value.tv_sec = 3;
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &timer, NULL);

        struct Clock *clock;
        int shm_id;
        int msqid;
        struct my_msgbuf rcvMessage;
        struct my_msgbuf sndMessage;
        key_t key = ftok("msgq_file.txt", 'B');
        msqid = msgget(key, 0666 | IPC_CREAT);

        shm_id = shmget(KEY,SIZE,0);
        if(shm_id < 0)
        {
                perror("shmget");
                exit(1);
        }

        clock = shmat(shm_id, NULL, 0);
        if(clock == (void *)-1)
        {
                perror("shmat");
                exit(1);
        }

        int memoryRef =0;
        int maxRef=1000;


        srand(getpid());
        int terminate = 0;
        int timeToCheckSec, timeToCheckNano;
        timeToCheckNano= rand()%250;
        timeToCheckNano+= clock->nano;
        timeToCheckSec=clock->sec;
        if(timeToCheckNano>=1000000)
        {
                timeToCheckNano=0;
                timeToCheckSec++;
        }

        int roll = rand()%100;
        if(roll>50)
        {
                maxRef+=100;
        }
        else
        {
                maxRef-=100;
        }

        while(terminate == 0)
        {

                //srand(getpid());
                if(timeToCheckNano<=clock->nano&&timeToCheckSec<=clock->sec)
                {



                        sndMessage.pageNum = rand()%32;
                        sndMessage.address =sndMessage.pageNum;
                        sndMessage.address *=1024;//converting page number request
                        int offset = rand()%1023;//adding offset to it
                        sndMessage.address +=offset;
                        sndMessage.mtype = getppid();
                        sndMessage.senderPid = getpid();
                        int randomNum = rand() % 100;
                        if(randomNum <=80)
                        {
                                sndMessage.data = 1;//read

                                //send request to oss
                                if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),IPC_NOWAIT)==-1)
                                {
                                        perror("msgsnd to parent failed\n");
                                        exit(1);
                                }

                                //wait for a response from oss
                                if(msgrcv(msqid, &rcvMessage, sizeof(rcvMessage),getpid(),0)==-1)
                                {
                                        perror("failed to receive message from parent\n");
                                        exit(1);
                                }
                                memoryRef++;

                        }
                        else
                        {
                                sndMessage.data = 2;//write

                                if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),IPC_NOWAIT)==-1)
                                {
                                        perror("msgsnd to parent failed\n");
                                        exit(1);
                                }

                                if(msgrcv(msqid, &rcvMessage, sizeof(rcvMessage),getpid(),0)==-1)
                                {
                                        perror("failed to receive message from parent\n");
                                        exit(1);
                                }

                                memoryRef++;

                        }

                        int termRandom = rand() %100;

                        //every 1000+-100 memory request
                        //terminate by chance;
                        if((termRandom<50)&&(memoryRef%maxRef==0))
                        {
                                terminate=1;
                                sndMessage.data=-1;
                                if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),IPC_NOWAIT)==-1)
                                {
                                        perror("msgsnd to parent failed\n");
                                        exit(1);
                                }

                                if(msgrcv(msqid, &rcvMessage, sizeof(rcvMessage),getpid(),0)==-1)
                                {
                                        perror("failed to receive message from parent\n");
                                        exit(1);
                                }

                        }

                        timeToCheckNano= rand()%250;
                        timeToCheckNano+= clock->nano;
                        timeToCheckSec=clock->sec;
                        if(timeToCheckNano>=1000000)
                        {
                                timeToCheckNano=0;
                                timeToCheckSec++;
                        }

                }
        }
}
