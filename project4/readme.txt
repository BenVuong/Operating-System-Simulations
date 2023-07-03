Project 4 created by Ben Vuong
Compile the project by using the make command
version control done by git. use the command git log to see the commit history

Purpose and functionality of this project:
This project will be simulating a scheduler and dispatcher
will fork and launch a worker process every 1 second or when both ready and block queue are empty
When a worker process is launched it will be loaded into the ready queue and will wait for it to recieve a message from oss
OSS will then dispatch a worker process from the front of the ready queue or block queue if ready queue is empty
When dispatching it will send the worker process a message, where message is the time quantum
Worker will then recieve that time quantum from the message and will randomly chooses three options
Option 1: Use up the entire time quantum it was given
        Will send back the entire time quantum to oss and oss load that process back into ready queueand worker will loop back and wait for oss to send it a message
Option 2: Use only part of the time quantum that it was given
        Will send back random value of 0 to time quantum and oss load that proces into block queue. Worker will loop back and wait for OSS to send it a message again
Option 3: Terminate
        Will send back random value of 0 to time quantum and an int value to indicate that it has terimated then exit

Through out the time that OSS runs, it will print to both screen and a log file.
Will print out stats like the ready and block queue, the PCB, the start time, total time in system, total cpu time, time in ready and block queue.
