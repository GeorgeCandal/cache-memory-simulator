# Introduction
This program is a cache simulator. You can specify number of sets, number of ways, and block size. It will read from a text file that simulates writes and reads of different memory addresses. It will simulate data movement from these memory addresses. At the end it will print out stats about hit rate, miss rate, eviction rate, data movement, and cache utilization.

# Dependencies
g++: C++ compiler  
make: To build the simulator

# Files
### Cache.h
Header file that contains the data structures used in the cache.

### Cache.cc
This contains the construction of the PLRU tree and the 5 cache operation methods (FindBlock, setBlock, getBlock, AccessBlock, and ReplaceBlock).

### Main.cc
This contains the main method. It reads from the trace file and simulates the cache by using the methods in Cache.cc

### Makefile
Used to compile and run the program.

### trace.txt
This is a text file of memory addresses used to simulate the cache.

# How to Run
 ```make run```   
This will compile the program to run with the default settings.   

```make run NUM_SETS=32 NUM_WAYS=8 BLOCK_SIZE=128```  
You can change the parameters with this command. Everything needs to be a power of 2, or the program will not run.    
