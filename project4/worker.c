//Author: Ben Vuong
////Date 3/21/2023
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
#include <sys/time.h>
#include <signal.h>

#define PERMS 0644
typedef struct msgbuffer {
        long mtype;
        int data;
        pid_t senderPID;
        int destination;//if 0 send to ready queue if 1 send to block queue and -1 if terminated

} msgbuffer;
void timeout_handler(int sig) {
    printf("\nTimeout occurred! Clearing out worker %d\n",getpid());
    exit(1);
}
int main(int argc, char *argv[])
{
        printf("Worker says hello\n");

        struct itimerval timer;
        signal(SIGALRM, timeout_handler);
        timer.it_value.tv_sec = 5;
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &timer, NULL);

        msgbuffer buf;
        buf.mtype = 1;
        int msqid = 0;
        key_t key;
        time_t t;

        if ((key = ftok("msgq.txt", 1)) == -1)
        {
                perror("ftok");
                exit(1);
        }

        if ((msqid = msgget(key, PERMS)) == -1) {
                perror("msgget in child");
                exit(1);
        }

        int flag = 0;
        while(flag == 0)//keep looping when worker uses up time quantum.
        {
                if ( msgrcv(msqid, &buf, sizeof(msgbuffer), getpid(), 0) == -1) {
                        perror("failed to receive message from parent\n");
                        exit(1);
                }

                printf("Worker received message: %d\n", buf.data);

                int timeQ = buf.data;
                buf.mtype = getppid();

                srand((unsigned) time(&t)*getpid());
                int num = rand()% 100 +1;
                if(num<=80 && num>=60)//uses all of time q
                {
                        buf.data = timeQ;
                        buf.senderPID = getpid();
                        buf.destination = 0;
                        if (msgsnd(msqid,&buf,sizeof(msgbuffer)-sizeof(long),0) == -1) {
                                perror("msgsnd to parent failed\n");
                                exit(1);
                        }

                }
                else if(num<=40)//terminate
                {
                        srand((unsigned) time(&t)*getpid());
                        buf.data = rand()% timeQ;
                        buf.senderPID = getpid();
                        buf.destination = -1;
                        if(msgsnd(msqid,&buf,sizeof(msgbuffer)-sizeof(long),0) == -1) {
                                perror("msgsnd to parent failed\n");
                                exit(1);
                        }
                        flag = 1;
                        printf("worker terminated\n");
                        exit(0);

                }
                else
                {
                        srand((unsigned) time(&t)*getpid());
                        buf.data = rand()% timeQ;
                        buf.senderPID = getpid();
                        buf.destination = 1;
                        if (msgsnd(msqid,&buf,sizeof(msgbuffer)-sizeof(long),0) == -1) {
                                perror("msgsnd to parent failed\n");
                                exit(1);
                        }


                }

        }
        exit(0);


}
