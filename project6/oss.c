//Author: Ben Vuong
//Date: 4/26/2023
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
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#define KEY 123
#define SIZE 100

struct PCB
{
        int occupied;
        pid_t pid;
        int pageTable[32];//each entry in this array will be what frame the page indexed is in
};

struct FrameTable
{
        int address;
        int pageNum;
        pid_t pid;
        int occupied;
        int dirtyBit;
        char headOfFIFO;
};

struct Clock
{
        unsigned int sec;
        unsigned int nano;
};

struct my_mgsbuf
{
        long mtype;
        int data;
        int pageNum;
        int address;
        pid_t senderPid;
};

void printMemoryMap(struct FrameTable frameTable[], struct PCB processTable[], FILE *file, int maxPCBSlots)
{
        printf("\nFrameTable:\n");
        fprintf(file,"\nFrameTable:\n");
        printf("Frame\toccupied\tPID\tPage\tBit\tHead\n");
        fprintf(file,"Frame\toccupied\tPID\tPage\tBit\tHead\n");
        for(int x =0; x<256; x++)
        {
                printf("%d\t%d\t\t%d\t%d\t%d\t%c\n",x,frameTable[x].occupied, frameTable[x].pid, frameTable[x].pageNum, frameTable[x].dirtyBit, frameTable[x].headOfFIFO);
                fprintf(file,"%d\t%d\t\t%d\t%d\t%d\t%c\n",x,frameTable[x].occupied, frameTable[x].pid, frameTable[x].pageNum, frameTable[x].dirtyBit, frameTable[x].headOfFIFO);
        }

        printf("\nPage Tables:\n");
        fprintf(file,"\nPage Tables:\n");
        for(int x = 0; x<maxPCBSlots; x++)
        {
                printf("PID: %d\n", processTable[x].pid);
                fprintf(file,"PID: %d\n", processTable[x].pid);
                for (int y=0; y<32; y++)
                {
                        printf("Page: %d Frame: %d\n", y, processTable[x].pageTable[y]);
                        fprintf(file,"Page: %d Frame: %d\n", y, processTable[x].pageTable[y]);
                }
                printf("\n");
                fprintf(file,"\n");
        }
        printf("\n");
        fprintf(file,"\n");

}

void timeout_handler(int sig)
{
        printf("Timeout occured! Will now be terminating child processif any are left\n");
        exit(1);
}



int main(int argc, char *argv[])
{
        printf("Hello this is OSS\n");
        FILE *file, *statFile;
        int maxPCBSlots = 20, pageFaults =0;
        struct PCB processTable[maxPCBSlots];
        struct FrameTable frameTable[256];

        struct itimerval timer;

        signal(SIGALRM, timeout_handler);

        timer.it_value.tv_sec = 2;
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;

        setitimer(ITIMER_REAL, &timer, NULL);

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


        if((statFile = fopen("stats.txt", "r")) == NULL)
        {
                printf("Stats file doesn't exist, creating stats file...\n");
                file = fopen("stats.txt", "w");
        }
        else
        {
                printf("Stats file found\n");
                fclose(file);
                file = fopen("stats.txt", "w");
        }


        int shmid;
        struct Clock *clock;
        system("touch msgq_file.txt");
        key_t key = ftok("msgq_file.txt",'B');
        struct my_mgsbuf sndMessage;
        struct my_mgsbuf rcvMessage;
        int msqid;
        msqid = msgget(key, 0666 | IPC_CREAT);
        shmid = shmget(KEY,SIZE, IPC_CREAT | 00666);
        if (shmid < 0)
        {
                perror("shmget");
                return 1;
        }

        clock = shmat(shmid, NULL, 0);
        if(clock == (struct Clock *)-1)
        {
                perror("shamt");
                return 1;
        }

        for(int x = 0; x<maxPCBSlots; x++)
        {
                processTable[x].pid = -1;
                processTable[x].occupied = 0;
                for (int y=0; y<32; y++)
                {
                        processTable[x].pageTable[y] = -1;
                }
        }

        for (int x = 0; x<256; x++)
        {
                //TODO: initalize the frame table
                frameTable[x].occupied = 0;
                frameTable[x].dirtyBit = 0;
                frameTable[x].pid = -1;
                frameTable[x].pageNum = -1;
                if(x == 0)
                {
                        frameTable[x].headOfFIFO = '*';
                }
                else
                {
                        frameTable[x].headOfFIFO = ' ';
                }

        }

        printf("OSS Launching\n\n");
        fprintf(file, "OSS Launching\n\n");
        clock->sec = 0;//initialize the clock
        clock->nano = 0;
        int index =0;
        int numLaunched = 0;
        int numRunning = 0;
        srand(time(NULL));
        int timeToLaunchSec=0, timeToLaunchNano=0,timeToPrintFrames=1 ;

        timeToLaunchNano=rand()%5000;
        timeToLaunchNano+=clock->nano;
        timeToLaunchSec = clock->sec;
        while(numLaunched<100)//terminated after 100 processes have been launched over time
        {
                clock->nano++;
                if (clock->nano >=1000000)
                {
                        clock->sec++;
                        clock->nano=0;
                }
                if(timeToPrintFrames == clock->sec)
                {
                        printf("Printing frame table and page tables at time %d:%d\n", clock->sec, clock->nano);
                        fprintf(file,"Printing frame table and page tables at time %d:%d\n", clock->sec, clock->nano);
                        timeToPrintFrames++;
                        printMemoryMap(frameTable, processTable, file, maxPCBSlots);

                }

                //timeToLaunchNano = rand() % 5000;
                if(timeToLaunchNano<=clock->nano&&timeToLaunchSec<=clock->sec&&numRunning<18)//only 18 can run at a time
                {
                        timeToLaunchNano = rand() %5000;//calc future time to fork
                        timeToLaunchNano+= clock->nano;
                        timeToLaunchSec = clock->sec;
                        if (timeToLaunchNano >=1000000)
                        {
                                timeToLaunchSec++;
                                timeToLaunchNano=0;
                        }


                        for(int a =0; a<maxPCBSlots; a++)//find an available space in PCB
                        {
                                if(processTable[a].occupied==0)
                                {
                                        index=a;
                                        break;

                                }
                                else
                                {
                                        index=-1;
                                }

                        }

                        if(index!=-1)
                        {
                                processTable[index].occupied = 1;
                                numLaunched++;
                                numRunning++;
                                printf("Launching a worker at %d:%d\n", clock->sec, clock->nano);
                                fprintf(file,"Launching a worker at %d:%d\n", clock->sec, clock->nano);
                                processTable[index].pid = fork();
                                if(processTable[index].pid==0)
                                {
                                        char *args[] = {"./worker",NULL};
                                        execvp(args[0], args);
                                }
                                else if(processTable[index].pid==-1)
                                {
                                        perror("fork");
                                        exit(1);

                                }
                        }
                        else
                        {
                                printf("No space in PCB\n");
                                fprintf(file, "No space in PCB\n");
                        }

                        //print PCB
                        printf("Entry\tOccupied\tPID\n");
                        fprintf(file,"Entry\tOccupied\tPID\n");
                        for(int i =0; i<maxPCBSlots;i++)
                        {

                                printf("%d\t%d\t\t%d\n",i,processTable[i].occupied, processTable[i].pid);
                                fprintf(file,"%d\t%d\t\t%d\n",i,processTable[i].occupied, processTable[i].pid);
                        }
                }


                //nonblocking wait for message from worker
                if(msgrcv(msqid, &rcvMessage, sizeof(rcvMessage),0,IPC_NOWAIT)==-1)
                {
                        if(errno != ENOMSG)
                        {
                                perror("msgrcv");
                                exit(1);
                        }

                }
                else
                {
                        int pageToLook;
                        //if oss recieves a 1 from worker then worker would like to read
                        //if oss receives a 2 from worker then worker would like to write
                        if(rcvMessage.data == 1)//read
                        {
                                printf("Worker: %d would like to read from Address: %d at time %d:%d\n", rcvMessage.senderPid, rcvMessage.address, clock->sec, clock->nano);
                                fprintf(file,"Worker: %d would like to read from Address: %d at time %d:%d\n", rcvMessage.senderPid, rcvMessage.address, clock->sec, clock->nano);
                                pageToLook = rcvMessage.address/1024;//translating address to page number
                                printf("Now going to look for page: %d\n",pageToLook);
                                fprintf(file,"Now going to look for page: %d\n",pageToLook);
                        }
                        else if(rcvMessage.data ==2)//write
                        {
                                printf("Worker: %d would like to write to Address: %d at time %d:%d\n", rcvMessage.senderPid, rcvMessage.address, clock->sec, clock->nano);
                                fprintf(file,"Worker: %d would like to write to Address: %d at time %d:%d\n", rcvMessage.senderPid, rcvMessage.address, clock->sec, clock->nano);
                                pageToLook = rcvMessage.address/1024;//translating address to page number
                                printf("Now going to look for page: %d\n",pageToLook);
                                fprintf(file,"Now going to look for page: %d\n",pageToLook);

                        }
                        else
                        {
                                //terminate and update PCB
                                printf("Worker: %d Terminated\n",rcvMessage.senderPid);
                                fprintf(file,"Worker: %d Terminated\n",rcvMessage.senderPid);
                                numRunning--;
                                for(int x=0; x<maxPCBSlots; x++)
                                {
                                        if(processTable[x].pid==rcvMessage.senderPid)
                                        {
                                                processTable[x].occupied=0;
                                                processTable[x].pid=-1;
                                                //TODO: clear out page table;
                                                for (int y = 0; y<32; y++)
                                                {
                                                        processTable[x].pageTable[y]=-1;
                                                }

                                                for (int z =0; z<256; z++)
                                                {
                                                        if(frameTable[z].pid == rcvMessage.senderPid)//free up space in frameTable when a process terminates
                                                        {
                                                                frameTable[z].occupied = 0;
                                                                frameTable[z].pageNum=-1;
                                                                frameTable[z].pid=-1;
                                                                frameTable[z].dirtyBit=0;

                                                        }
                                                }

                                                sndMessage.data=1;
                                                sndMessage.mtype=rcvMessage.senderPid;
                                                if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),0) ==-1)
                                                {
                                                        perror("msgsnd to worker failed\n");
                                                }
                                                break;

                                        }
                                }
                        }


                        //TODO: Find page within frame Table and implement clock policy for replacement
                        if(rcvMessage.data !=-1)//only do it if worker wants to read or write
                        {
                                int frameIndex=0, found=0;

                                for (int x=0; x<256; x++)
                                {
                                        if(frameTable[x].pid==rcvMessage.senderPid && frameTable[x].pageNum == pageToLook)
                                        {
                                                found =1;
                                                frameTable[x].dirtyBit=1;
                                                frameIndex=x;
                                                printf("Page %d of PID: %d found in frame: %d\n",pageToLook,rcvMessage.senderPid,x);
                                                fprintf(file,"Page %d of PID: %d found in frame: %d\n",pageToLook,rcvMessage.senderPid,x);
                                                break;
                                        }
                                }
                                if (found == 1)
                                {
                                        sndMessage.mtype = rcvMessage.senderPid;
                                        sndMessage.data = 1;

                                        if(rcvMessage.data == 1)
                                        {
                                                printf("Address: %d in frame: %d, giving data to Worker: %d at time %d:%d\n",rcvMessage.address,frameIndex,rcvMessage.senderPid,clock->sec,clock->nano);
                                                fprintf(file,"Address: %d in frame: %d, giving data to Worker: %d at time %d:%d\n",rcvMessage.address,frameIndex,rcvMessage.senderPid,clock->sec,clock->nano);
                                        }
                                        else
                                        {
                                                printf("Address: %d in frame: %d, writing data to frame at time %d:%d\n",rcvMessage.address,frameIndex,clock->sec,clock->nano);
                                                fprintf(file,"Address: %d in frame: %d, writing data to frame at time %d:%d\n",rcvMessage.address,frameIndex,clock->sec,clock->nano);
                                        }
                                        if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),0) ==-1)
                                        {
                                                perror("msgsnd to worker failed\n");
                                        }

                                }
                                else
                                {
                                        for (int x =0; x<256; x++)
                                        {


                                                if(frameTable[x].occupied == 0)
                                                {
                                                        frameTable[x].occupied =1;
                                                        frameTable[x].pageNum = pageToLook;
                                                        frameTable[x].pid = rcvMessage.senderPid;
                                                        frameTable[x].dirtyBit = 1;

                                                        clock->nano+=14000;
                                                        if(clock->nano == 1000000)
                                                        {
                                                                clock->nano=0;
                                                                clock->sec++;
                                                        }
                                                        printf("Inserting page: %d in frame: %d\n",pageToLook, x );
                                                        fprintf(file,"Inserting page: %d in frame: %d\n",pageToLook, x );
                                                        frameIndex=x;
                                                        for(int y=0; y<maxPCBSlots; y++)
                                                        {
                                                                if(processTable[y].pid == rcvMessage.senderPid)
                                                                {
                                                                        processTable[y].pageTable[pageToLook]=x;
                                                                        sndMessage.mtype = rcvMessage.senderPid;
                                                                        sndMessage.data = 1;
                                                                        if(rcvMessage.data == 1)
                                                                        {
                                                                                printf("Address: %d in frame: %d, giving data to Worker: %d at time %d:%d\n",rcvMessage.address,x,rcvMessage.senderPid,clock->sec,clock->nano);
                                                                                fprintf(file,"Address: %d in frame: %d, giving data to Worker: %d at time %d:%d\n",rcvMessage.address,x,rcvMessage.senderPid,clock->sec,clock->nano);
                                                                        }
                                                                        else
                                                                        {
                                                                                printf("Address: %d in frame: %d, writing data to frame at time %d:%d\n",rcvMessage.address,x,clock->sec,clock->nano);
                                                                                fprintf(file,"Address: %d in frame: %d, writing data to frame at time %d:%d\n",rcvMessage.address,x,clock->sec,clock->nano);
                                                                        }
                                                                        if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),0) ==-1)
                                                                        {
                                                                                perror("msgsnd to worker failed\n");
                                                                        }
                                                                        break;
                                                                }
                                                        }
                                                        break;
                                                }

                                                else
                                                {
                                                        frameIndex=-1;
                                                }
                                        }
                                        //TODO: When all the spaces in the frame table is taken up use clock policy replacment aglorithm to find swap out memory.
                                        int pointer = 0;
                                        if(frameIndex == -1)
                                        {
                                                printf("Could not find page: %d of PID: %d and no space in main memory\n", pageToLook, rcvMessage.senderPid);
                                                //find the current head of FIFO
                                                for(int x = 0; x<256; x++)
                                                {
                                                        if(frameTable[x].headOfFIFO=='*')
                                                        {
                                                                pointer = x;
                                                                break;
                                                        }
                                                }

                                                //loop through frame table to find available space to swap out
                                                for(int y=pointer; y< 256; y++)
                                                {
                                                        frameTable[y].headOfFIFO='*';
                                                        if(frameTable[y].dirtyBit == 1)
                                                        {
                                                                frameTable[y].dirtyBit=0;
                                                                frameTable[y].headOfFIFO=' ';
                                                        }
                                                        else
                                                        {
                                                                int tempPage, tempFrame;
                                                                pid_t tempPid;
                                                                for(int z=0; z< maxPCBSlots; z++)//clearing processes frame number
                                                                {
                                                                        if(frameTable[y].pid == processTable[z].pid)
                                                                        {
                                                                                processTable[z].pageTable[frameTable[y].pageNum]=-1;

                                                                        }
                                                                //      break;
                                                                }

                                                                frameTable[y].dirtyBit=1;
                                                                frameTable[y].pid=rcvMessage.senderPid;
                                                                frameTable[y].pageNum=pageToLook;
                                                                frameTable[y].headOfFIFO='*';

                                                                for(int z =0; z< maxPCBSlots; z++)//update process page table with new frame number
                                                                {
                                                                        if(frameTable[y].pid == processTable[z].pid)
                                                                        {
                                                                                processTable[z].pageTable[frameTable[y].pageNum]=y;
                                                                        }
                                                                }
                                                                printf("Clearing out frame %d and swapping in PID: %d page: %d\n",y, rcvMessage.senderPid, pageToLook);
                                                                fprintf(file,"Clearing out frame %d and swapping in PID: %d page: %d\n",y, rcvMessage.senderPid, pageToLook);

                                                                clock->nano+=14000;
                                                                if(clock->nano == 1000000)
                                                                {
                                                                        clock->nano=0;
                                                                        clock->sec++;
                                                                }


                                                                sndMessage.mtype = rcvMessage.senderPid;
                                                                sndMessage.data = 1;
                                                                if(rcvMessage.data == 1)
                                                                {
                                                                        printf("Address: %d in frame: %d, giving data to Worker: %d at time %d:%d\n",rcvMessage.address,y,rcvMessage.senderPid,clock->sec,clock->nano);
                                                                        fprintf(file,"Address: %d in frame: %d, giving data to Worker: %d at time %d:%d\n",rcvMessage.address,y,rcvMessage.senderPid,clock->sec,clock->nano);
                                                                }
                                                                else
                                                                {
                                                                        printf("Address: %d in frame: %d, writing data to frame at time %d:%d\n",rcvMessage.address,y,clock->sec,clock->nano);
                                                                        fprintf(file,"Address: %d in frame: %d, writing data to frame at time %d:%d\n",rcvMessage.address,y,clock->sec,clock->nano);
                                                                }
                                                                if(msgsnd(msqid, &sndMessage, sizeof(sndMessage),0) ==-1)
                                                                {
                                                                        perror("msgsnd to worker failed\n");
                                                                }
                                                                pageFaults++;
                                                                double pfs=0.0;//pagefualts per seconds
                                                                pfs = (double) pageFaults/clock->sec;
                                                                printf("\nNumber of page faults: %d\n", pageFaults);
                                                                printf("Page faults per second: %.2f\n\n", pfs);

                                                                fprintf(file,"\nNumber of page faults: %d\n", pageFaults);
                                                                fprintf(file,"Page faults per second: %.2f\n\n", pfs);

                                                                //      fprintf(statFile,"Number of page faults: %d\n", pageFaults);
                                                                break;
                                                        }
                                                        if(y==255)//make it a circular array
                                                        {
                                                                y=-1;
                                                        }
                                                }
                                        }
                                }
                        }
                }



        }

        fclose(statFile);
        fclose(file);
        if (msgctl(msqid, IPC_RMID, NULL) == -1) {
                perror("msgctl to get rid of queue in parent failed");
                exit(1);
        }
        return 0;
}

