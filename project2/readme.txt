To compile this program use the make command
to run the program run the oss executable
when launching oss you will need to give it 3 arugments -n, -s, -t.
-n is for the total number of processes to launch
-s is for how many processes would run simultatinously
-t is the max time limit for the worker process to use.
All three arugements needs to be present for oss to run.
use the -h arguemnt to get a help message.

For this program, oss is running a simulated system clock that is in shared memory with seconds and nanoseconds.
when workers are launched, it will be given a random time dictated by maxTimeLimit argument given
to oss. Workers will periodiclly look at the oss system clock in shared memory. Once they have found that the time limit has passed compared to the system clock, it will terminate.

Used git for version control.
Use the command git log to see the commits that were made.

Citations:
https://www.geeksforgeeks.org/ipc-shared-memory/
