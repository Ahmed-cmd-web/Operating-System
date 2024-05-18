#include <stdio.h>
#include <stdbool.h>

typedef struct {
    int PID;
    bool state;  // blocked/unblocked
    int priority;
    int pc;
    int memoryBoundaries[2];   // [startInMemory, endInMemory]
} PCB;


char* Memory[57];
PCB PCBMemory[3];
PCB generalBlockedQueue[3];  // 3 queues for the 3 resources {Accessing file,Taking user input,Outputting }   True->blocked, False->unblocked
PCB queues[4][3];  // 4 queues for the 4 priorities
bool mutexes[3];  // 3 mutexes for the 3 resources {Accessing file,Taking user input,Outputting }   True->blocked, False->unblocked
PCB blockedResources[3][3];  // 3 queues for the 3 resources {Accessing file,Taking user input,Outputting }

void read;
void execute;

int main()
{

    return 0;
}






//
// [_,_,_,_,_]   ->Memory

// reading program 1
// [semWait userInput,assign a input ,.....] -> program 1
//   1                     2           3
// [PCB1] -> PCBMemory
// [[PCB1],[],[],[]] -> queues
//  1 2 3 4

// execute line 1 of program 1
// [[],[PCB1],[],[]] -> queues
// [semWait userInput,assign a input ,.....] -> program 2
//   12                    13          14
// [PCB1,PCB2] -> PCBMemory



// reading the program
    // inserting into the memory
    // creating a PCB instance
    // inserting the PCB instance into the PCBMEMORY
    // inserting the PCB instance into the queues


// executing the program
    // pick the highest priority PCB from queues
    // execute the next instruction based on the current quantum
    // if the instruction is a system call(read input,open file,outputing),
        // if the resource is available, execute the instruction and set the mutex respectively
        // if the resource is not available, block the PCB by enqueuing it to the generalBlockedQueue and the respective blockedQueue of the resource
    // finally push the PCB to the next lower quantum
