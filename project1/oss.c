//Author: Ben Vuong
//Date: 1/30/2023
//Will take in 3 arugments -n, -s, -t
//-n will be used to launch n numbner of procresses which is the worker executable
//-s will be the max amount of processess will be deployed simultaneously
//-t will be the number iterations that will be passed on to worker
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <getopt.h>
int main(int argc, char *argv[])
{
        int opt;
        int requiredArg1 = 0;
        int requiredArg2 = 0;
        int requiredArg3 = 0;

        int inter;
        char *number;
        char *maxSim;
        char *iter;
        int sim = 0;

        while ((opt = getopt(argc, argv, "hn:s:t:")) !=-1)
        {
                switch (opt)
                {
                        case 'h':
                                printf("Help option selected. \n");
                                printf("use the -n argument to launch that number of processes\n");
                                printf("use the -s argument to limit how many process can run simultaneously\n");
                                printf("use the -t arugment to give the number iterations the worker executable will use\n");
                                break;
                        case 'n':
                                printf("number option selected. '%s' \n", optarg);
                                requiredArg1 = 1;
                                number =optarg;
                                break;
                        case 's':
                                printf("simul option selected. '%s' \n", optarg);
                                requiredArg2 = 1;
                                maxSim = optarg;
                                break;
                        case 't':
                                printf("iterations option selected. '%s' \n", optarg);
                                requiredArg3 = 1;
                                iter = optarg;
                                break;
                        default:
                                printf("Invalid option.\n");
                                break;
                }
        }

        if(requiredArg1 && requiredArg2 && requiredArg3)
        {
                printf("All arguments are present\n");
                printf("Will launch: %d worker processes\n",atoi(number));
                printf("Each worker processes will iterate: %d times\n",atoi(iter));
                printf("Only %d process can be launched simultaniously\n",atoi(maxSim));
                int processCount = 0;
                int status;

        //      while(processCount < atoi(number))
        //      {
                        while(sim < atoi(maxSim) &&  processCount < atoi(number))
                        {
                                sim++;
                                processCount++;
                                if (fork()==0)//Fork off one process
                                {
                                        processCount++;
                                        char *name ="worker";
                                        execl("worker",name,iter,NULL);// execute worker
                                        printf("Error");
                                }
                                if (sim == atoi(maxSim))
                                {
                                        wait(NULL);
                                        sim--;
                                }




                        }
        //      }


        }

        else
        {
                printf("Must provide arugments: -n -s -t\n");
        }
        return 0;

}
