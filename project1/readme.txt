To compile this program use the make command
To run the program run the oss executeable.
When launching oss you will need to give it 3 arugments -n, -s, -t.
All three arguements needs to be present for oss to run.
Use the -h arugement to get a help message.

When OSS is given all 3 arugments, it will fork off -n child processes and those will execute the worker executable. Those workers will iterate -t times. Those only -s workers are able to run at the same time.

Note: currently OSS works fine. Though there is a bug where if the total number process is divisable by the max number of simultanious processes allowed to run, the last remainder processes will be orphan child process with a PPID of 1. However if the the total number of processes is divisable by the max number of simultanious process allowed to run, it will run fine with no orphaned child processes.

Git was used for version control. Please use the command git log to see the commits made.

Citations:
https://www.geeksforgeeks.org/create-n-child-process-parent-process-using-fork-c/
https://www.geeksforgeeks.org/fork-system-call/
https://www.geeksforgeeks.org/creating-multiple-process-using-fork/
https://www.thegeekdiary.com/how-to-use-execl-example-included/
