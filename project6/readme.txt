//Author: Ben Vuong
//Date: 4/26/2023

Git was used as version control.
To see the commits made use the command git log
To compile this program use the command make

Project's Task:
At random time fork a child worker process with only 18 worker processes can be running at a time
At random time a child worker process will send a message to oss with an address and if it want to read or write to that address
oss will then take that address then convert that to page number
Frame table will be used to track what pid and its page number/address will be stored in what frame
Each worker process will have its own page table to track which of its page is in which frame
Clock policy is used as the replacement policy
