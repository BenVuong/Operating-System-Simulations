//Author: Ben Vuong
//Date 3/21/2023
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
#include <sys/queue.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#define MAX_CHILDREN 10
#define KEY 123
#define SIZE 100
#define PERMS 0644
struct PCB
{
        int occupied; // either ture or false
        pid_t pid; //process id of this child
        int startSeconds; //time when it was forked
        int startNano; //time when it was forked
        int totalCPUTimeSec;
        int totalTimeInSystemSec;
        int totalCPUTimeNano;
        int totalTimeInSystemNano;
        int timeSpentInBlockQSec;
        int timeSpentInBlockQNano;
        int timeSpentInReadyQSec;
        int timeSpentInReadyQNano;

};

struct Clock
{
        int seconds;
        int nanoSeconds;
};

typedef struct msgbuffer
{
        long mtype;
        int data;
        pid_t senderPID;
        int destination;
} msgbuffer;

typedef struct //create struct that will be used for ready queue and blocked queue
{
        pid_t* arr;
        int front;
        int rear;
        int size;
        int capacity;


} Queue;

Queue* createQueue(int capacity)
{
        Queue* queue = (Queue*) malloc(sizeof(Queue));
        queue->arr = (pid_t*) malloc(capacity * sizeof(pid_t));
        queue->front = 0;
        queue->rear=-1;
        queue->size = 0;
        queue->capacity = capacity;
        return queue;
}

int isFull(Queue* queue)
{
        return (queue->size == queue->capacity);
}

int isEmpty(Queue* queue) {
        return (queue->size == 0);
}


void enqueue(Queue* queue, pid_t element)
{
        if (isFull(queue))
        {
                printf("Queue is Full. Cannot enqueue. \n");
                return;
        }
        queue->rear = (queue->rear+1) % queue->capacity;
        queue->arr[queue->rear] = element;
        queue->size++;
}

pid_t dequeue(Queue* queue) {
        if (isEmpty(queue)) {
                printf("Queue is empty. Cannot dequeue.\n");
                return -1;
        }
        pid_t element = queue->arr[queue->front];
        queue->front = (queue->front + 1) % queue->capacity;
        queue->size--;
        return element;
}

void printQueue(Queue* queue, FILE *file) {
        printf("Queue: ");
        fprintf(file,"Queue: ");
        int i;
        for (i = 0; i < queue->size; i++) {
                int index = (queue->front + i) % queue->capacity;
                printf("%d ", queue->arr[index]);
                fprintf(file,"%d ", queue->arr[index]);
        }
        printf("\n");
        fprintf(file,"\n");

}



pid_t peek(Queue* queue) {
        if (isEmpty(queue)) {
                printf("Queue is empty. Cannot peek.\n");
                return -1;
        }
        return queue->arr[queue->front];
}


void timeout_handler(int sig)
{
        printf("Timeout occured! Will now be terminating child processes if any are left\n");
        exit(1);
}

int main(int argc, char *argv[])
{
        Queue* readyQueue = createQueue(10);
        Queue* blockQueue = createQueue(10);
        FILE *file;
        int maxPCBSlots = 10;
        struct PCB processTable[maxPCBSlots];
        int clockSec = 0;
        int clockNano = 0;
        int cpuTimeSpentSec = 0;
        int cpuTimeSpentNano = 0;
        int timeQ = 100000; //time quantum is 100,000 nano seconds

        struct itimerval timer;

        signal(SIGALRM, timeout_handler);

        timer.it_value.tv_sec = 3;
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;

        setitimer(ITIMER_REAL, &timer, NULL);

        for(int x = 0; x < maxPCBSlots; x++)//set up the PCB with default values
        {
                processTable[x].pid = -2;//default value for empty pcb slots
                processTable[x].occupied = 0;
                processTable[x].startSeconds = 0;
                processTable[x].startNano = 0;
                processTable[x].totalCPUTimeSec=0;
                processTable[x].totalTimeInSystemSec=0;
                processTable[x].totalCPUTimeNano=0;
                processTable[x].totalTimeInSystemNano=0;
                processTable[x].timeSpentInBlockQSec = 0;
                processTable[x].timeSpentInBlockQNano = 0;
                processTable[x].timeSpentInReadyQSec = 0;
                processTable[x].timeSpentInReadyQNano = 0;

        }

        msgbuffer message;
        int msqid;
        key_t key;
        system("touch msgq.txt");

        if((key = ftok("msgq.txt", 1)) ==-1)
        {
                perror("ftok");
                exit(1);
        }

        if((msqid = msgget(key, PERMS | IPC_CREAT)) == -1)
        {
                perror("msgget in parent");
                exit(1);
        }
        int index;
        int timeSinceLastFork = 0;
        printf("OSS with PID: %d  says hello\n", getpid());
        int processGeneratedCount = 0;

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

        int timeSpentNoReadySec=0;
        int timeSpentNoReadyNano=0;

        while(processGeneratedCount < 100)
        {
                int accumulatedTime = 0;
                if((isEmpty(readyQueue)&&(isEmpty(blockQueue)))||(clockNano - timeSinceLastFork) >= 25000 || (clockNano-timeSinceLastFork) == 0 )
                {

                        if(isFull(readyQueue)==0)
                        {
                                for (int a = 0; a < maxPCBSlots; a++)//to find an available space in pcb
                                {
                                        if(processTable[a].occupied == 0)
                                        {
                                                index = a;
                                                break;
                                        }
                                        else
                                        {
                                                index = -1;
                                        }
                                }
                                if(index != -1)
                                {

                                        timeSinceLastFork = clockNano;
                                        processTable[index].occupied = 1;
                                        processTable[index].startNano = clockNano;
                                        processTable[index].startSeconds = clockSec;
                                        processTable[index].totalCPUTimeNano = 0;
                                        processTable[index].totalCPUTimeSec = 0;
                                        clockNano+=200000;//time it takes to load into ready queue
                                        processTable[index].timeSpentInBlockQSec = 0;
                                        processTable[index].timeSpentInBlockQNano = 0;
                                        processTable[index].timeSpentInReadyQSec = 0;
                                        processTable[index].timeSpentInReadyQNano = 0;

                                        accumulatedTime+=200000;
                                        if(clockNano >= 1000000)//convert nanoseconds to seconds for clock
                                        {
                                                clockSec++;
                                                clockNano = 0;
                                        }
                                        processGeneratedCount++;
                                        processTable[index].pid = fork();
                                        if(processTable[index].pid == 0)
                                        {
                                                char *args[] = {"./worker", NULL};
                                                execvp(args[0], args);
                                        }
                                        else if(processTable[index].pid ==-1)
                                        {
                                                perror("fork");
                                                exit(1);
                                        }
                                        else
                                        {
                                                enqueue(readyQueue, processTable[index].pid);
                                                printf("Printing Ready ");
                                                fprintf(file,"Printing Ready ");
                                                printQueue(readyQueue,file);
                                                printf("Printing Block ");
                                                fprintf(file,"Printing Block ");
                                                printQueue(blockQueue,file);

                                                printf("OSS: Generating Worker process with PID %d located in index %d of the pcb and putting it in ready queue at time %d:%d\n", processTable[index].pid,index,clockSec,clockNano);
                                                fprintf(file,"OSS: Generating Worker process with PID %d located in index %d of the pcb and putting it in ready queue at time %d:%d\n", processTable[index].pid,index,clockSec,clockNano);


                                        }



                                }
                                else
                                {
                                        timeSinceLastFork = clockNano;
                                        printf("No space in PCB\n");
                                        fprintf(file,"No space in PCB\n");

                                }
                        }
                }


                //dispatching
                if(isEmpty(readyQueue) == 0) //if ready queue is not empty then dequeue and dispatch worker process with the given pid
                {
                        message.mtype = dequeue(readyQueue); //dispatching worker
                        message.data = timeQ;
                        clockNano+=100000;
                        accumulatedTime+=100000;
                        if(clockNano>=1000000)
                        {
                                clockSec++;
                                clockNano=0;
                        }

                        printf("OSS: Dispatching worker process with PID %d from ready queue at time %d:%d\n",message.mtype,clockSec,clockNano);
                        fprintf(file,"OSS: Dispatching worker process with PID %d from ready queue at time %d:%d\n",message.mtype,clockSec,clockNano);

                        if (msgsnd(msqid, &message, sizeof(msgbuffer)-sizeof(long),0)==-1)
                        {
                                perror("msgsnd to worker failed\n");
                                exit(1);
                        }

                }
                else//if ready queue is empty dispatch a process from blocked queue
                {
                        message.mtype = dequeue(blockQueue);
                        message.data = timeQ;
                        clockNano+=10000;
                        accumulatedTime+=100000;
                        if(clockNano>=1000000)
                        {
                                clockSec++;
                                clockNano=0;
                        }


                        printf("OSS: Ready queue is empty now dispatching from blocked queue\n");
                        fprintf(file,"OSS: Ready queue is empty now dispatching from blocked queue\n");
                        printf("OSS: Dispatching worker process with PID %d from block queue at time %d,%d\n",message.mtype,clockSec,clockNano);
                        fprintf(file,"OSS: Dispatching worker process with PID %d from block queue at time %d,%d\n",message.mtype,clockSec,clockNano);

                        if (msgsnd(msqid, &message, sizeof(msgbuffer)-sizeof(long),0)==-1)
                        {
                                perror("msgsnd to worker failed\n");
                                exit(1);
                        }

                }


                msgbuffer rcvbuf;
                //receieve message back from a worker process
                if(msgrcv(msqid, &rcvbuf, sizeof(msgbuffer), getpid(),0)==-1)
                {
                        perror("failed to recevive message in parent\n");
                        exit(1);
                }
                printf("OSS: Receiving that worker process with PID %d ran for %d nanoseconds\n",rcvbuf.senderPID, rcvbuf.data);
                fprintf(file,"OSS: Receiving that worker process with PID %d ran for %d nanoseconds\n",rcvbuf.senderPID, rcvbuf.data);

                accumulatedTime+=rcvbuf.data;
                clockNano+=rcvbuf.data;
                cpuTimeSpentNano +=rcvbuf.data;
                if(cpuTimeSpentNano >= 1000000)
                {
                        cpuTimeSpentSec++;
                        cpuTimeSpentNano=0;
                }
                for(int j = 0; j < maxPCBSlots;j++)
                {
                        if(processTable[j].pid == rcvbuf.senderPID)
                        {
                                processTable[j].totalCPUTimeNano+=rcvbuf.data;
                                if(processTable[j].totalCPUTimeNano >= 1000000)
                                {
                                        processTable[j].totalCPUTimeSec++;
                                        processTable[j].totalCPUTimeNano=0;
                                }
                        }
                }

                if(clockNano>=1000000)
                {
                        clockSec++;
                        clockNano=0;
                }



                if(rcvbuf.destination == 0)//will load process into ready queue
                {
                        printf("OSS: Used up time quantum, putting worker process with PID %d back into ready queue\n",rcvbuf.senderPID);
                        fprintf(file,"OSS: Used up time quantum, putting worker process with PID %d back into ready queue\n",rcvbuf.senderPID);

                        enqueue(readyQueue, rcvbuf.senderPID);
                        printf("Printing Ready ");
                        fprintf(file,"Printing Ready ");
                        printQueue(readyQueue,file);
                        printf("Printing Block ");
                        fprintf(file,"Printing Block ");
                        printQueue(blockQueue,file);

                }
                else if (rcvbuf.destination == 1)//will load process into block queue
                {
                        printf("OSS: Did not use up time quantum, putting worker process with PID %d back into block queue\n",rcvbuf.senderPID);
                        fprintf(file,"OSS: Did not use up time quantum, putting worker process with PID %d back into block queue\n",rcvbuf.senderPID);

                        enqueue(blockQueue, rcvbuf.senderPID);
                        printf("Printing Ready ");
                        fprintf(file,"Printing Ready ");
                        printQueue(readyQueue,file);
                        printf("Printing Block ");
                        fprintf(file,"Printing Block ");
                        printQueue(blockQueue,file);

                }
                else//process terminated so update pcb to free up space in pcb
                {
                        printf("OSS: process with PID %d terminated\n",rcvbuf.senderPID);
                        fprintf(file,"OSS: process with PID %d terminated\n",rcvbuf.senderPID);
                        for (int x = 0; x < maxPCBSlots; x++)
                        {
                                if(processTable[x].pid == rcvbuf.senderPID)//given the pid find the index where the process is stored in the pcb
                                {
                                        processTable[x].occupied = 0;
                                }
                        }

                }
                //Update PCB
                int totalBlockTime, totalReadyTime;
                double avgBlockTime, avgReadyTime;
                if(isEmpty(blockQueue) == 0)//loop through block queue to update the time spent in block queue stored in pcb
                {
                        totalBlockTime = 0;
                        for (int b = 0; b < blockQueue->size; b++)
                        {
                                int p = (blockQueue->front + b) % blockQueue->capacity;
                                for (int x = 0; x <maxPCBSlots; x++)
                                {
                                        if(processTable[x].pid == blockQueue->arr[p])
                                        {
                                                processTable[x].timeSpentInBlockQNano+=accumulatedTime;
                                                if(processTable[x].timeSpentInBlockQNano >=1000000)
                                                {
                                                        processTable[x].timeSpentInBlockQSec++;
                                                        processTable[x].timeSpentInBlockQNano=0;
                                                }
                                                totalBlockTime+=processTable[x].timeSpentInBlockQSec;

                                        }
                                }

                        }
                          avgBlockTime = (double) totalBlockTime/blockQueue->size;
                }

                if(isEmpty(readyQueue) == 0)//loop through ready queue to update the time spent in ready queue stored in pcb
                {
                        totalReadyTime = 0;
                        for (int b = 0; b < readyQueue->size; b++)
                        {
                                int p = (readyQueue->front + b) % readyQueue->capacity;
                                for (int x = 0; x <maxPCBSlots; x++)
                                {
                                        if(processTable[x].pid == readyQueue->arr[p])
                                        {
                                                processTable[x].timeSpentInReadyQNano+=accumulatedTime;
                                                if(processTable[x].timeSpentInReadyQNano >=1000000)
                                                {
                                                        processTable[x].timeSpentInReadyQSec++;
                                                        processTable[x].timeSpentInReadyQNano=0;
                                                }
                                                totalReadyTime+=processTable[x].timeSpentInReadyQSec;

                                        }
                                }

                        }
                          avgReadyTime = (double) totalReadyTime/readyQueue->size;
                }


                //Print PCB
                printf("Entry\tOccupied\tPID\tStartS\tStartN\ttotalCPUTimeSec\ttotalCPUTimeNano\n");
                fprintf(file,"Entry\tOccupied\tPID\tStartS\tStartN\ttotalCPUTimeSec\ttotalCPUTimeNano\n");
                for(int i =0; i<maxPCBSlots;i++)
                {

                        printf("%d\t%d\t\t%d\t%d\t%d\t%d\t\t%d\n",i,processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano, processTable[i].totalCPUTimeSec, processTable[i].totalCPUTimeNano);


                        fprintf(file,"%d\t%d\t\t%d\t%d\t%d\t%d\t\t%d\n",i,processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano, processTable[i].totalCPUTimeSec, processTable[i].totalCPUTimeNano);

                }
                if(isEmpty(readyQueue))
                {

                        timeSpentNoReadyNano+=accumulatedTime;
                        if(timeSpentNoReadyNano >= 1000000)
                        {
                                timeSpentNoReadySec++;
                                timeSpentNoReadyNano=0;
                        }

                }
                else
                {
                //       timeSpentNoReadySec=0;
                //       timeSpentNoReadyNano=0;

                }

                printf("Average time spent waiting in block Queue: %.2f Seconds\n",avgBlockTime);
                fprintf(file,"Average time spent waiting in block Queue: %.2f Seconds\n",avgBlockTime);
                printf("Average time spent waiting in ready Queue: %.2f Seconds\n",avgReadyTime);
                fprintf(file,"Average time spent waiting in ready Queue: %.2f Seconds\n",avgReadyTime);
                printf("OSS: Processes generated: %d\n", processGeneratedCount);
                fprintf(file,"OSS: Processes generated: %d\n", processGeneratedCount);
                printf("Total time spent on CPU %d:%d\n",cpuTimeSpentSec, cpuTimeSpentNano);
                fprintf(file,"Total time spent on CPU %d:%d\n",cpuTimeSpentSec, cpuTimeSpentNano);
                printf("Total time %d:%d\n",clockSec,clockNano);
                fprintf(file,"Total time %d:%d\n",clockSec,clockNano);
                double ultilization =( (double)cpuTimeSpentSec/clockSec)*100;
                printf("Ultilization: %.2f% \n",ultilization);
                fprintf(file,"Ultilization: %.2f% \n",ultilization);
                printf("Time Spent with no processes in ready Queue, %d:%d\n",timeSpentNoReadySec, timeSpentNoReadyNano);
                fprintf(file,"Time Spent with no processes in ready Queue, %d:%d\n",timeSpentNoReadySec, timeSpentNoReadyNano);
                printf("\n");
                fprintf(file,"\n");
        }


                wait(0);
                fclose(file);
        if (msgctl(msqid, IPC_RMID, NULL) == -1) {
                perror("msgctl to get rid of queue in parent failed");
                exit(1);
        }

}
