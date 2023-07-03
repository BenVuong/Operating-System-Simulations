To compile this project, use the make command
to run the program run the oss executable
when launching oss you will need to give it 3 arguments -n, -s, -t, and -f
-n is for the total number of processes to launch
-s is for how many processes would run simultatinously
-t is the max time limit for the worker process to use.
-f will enable outputs from oss to be printed to the screen and a log file
use the -h arguemnt to get a help message.
For this program, oss is running a simulated system clock that is in shared memory with seconds and nanoseconds.
when workers are launched, it will be given a random time dictated by maxTimeLimit argument given
to oss. Workers will periodiclly look at the oss system clock in shared memory. Once they have found that the time limit has passed compared to the system clock, it will terminate.

For this project instead of oss giving the the worker processses the arguments via command line arugments
they are given via a message queue. oss will use msgsnd and worker will do a  msgrcv() call to receive it.

git was used for version control.
Use git log command to see the different commits made.

citations:
https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_message_queues.htm
