#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "utils.c"
#include "operations.c"


PCB* read(char* filename){
    FILE *file = fopen(filename, "r");
    PCB* newPCB=(PCB*)malloc(sizeof(PCB));

    newPCB->PID = currentPID++;
    Memory[lastMemoryPosition++] = createPCBattr("PID", newPCB->PID);


    newPCB->state = 0;
    Memory[lastMemoryPosition++] = createPCBattr("State", newPCB->state);

    newPCB->priority = 0;
    Memory[lastMemoryPosition++] = createPCBattr("Priority", newPCB->priority);


    newPCB->pc = 0;
    Memory[lastMemoryPosition++] = createPCBattr("PC", newPCB->pc);


    newPCB->memoryBoundaries[0] = lastMemoryPosition-4;
    Memory[lastMemoryPosition++] = createPCBattr("lowerBound", newPCB->memoryBoundaries[0]);


    int previousMemoryPosition = lastMemoryPosition;
    char line[100];
    while (fgets(line, 100, file) != NULL){
        word* newWord = (word*)malloc(sizeof(word));
        line[strcspn(line, "\r\n")]='\0';
        newWord->name = "Instruction";
        newWord->value = strdup(line);
        Memory[lastMemoryPosition+4] = newWord;
        lastMemoryPosition++;
    }


    fclose(file);
    newPCB->memoryBoundaries[1] = lastMemoryPosition+4;
    Memory[previousMemoryPosition++] = createPCBattr("upperBound", newPCB->memoryBoundaries[1]);

    Memory[previousMemoryPosition++] = NULL;
    Memory[previousMemoryPosition++] = NULL;
    Memory[previousMemoryPosition++] = NULL;


    lastMemoryPosition++;

    return newPCB;
}



PCB* getHighestPriorityUnblocked(int* quantum){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            PCB* currentPCB = queues[i][j];
            if (currentPCB != NULL && currentPCB->state == 0) {
                    *quantum = i+1;
                    return currentPCB;
            }
        }
    }
    return NULL;
}




void execute(){
    int quantum;
    PCB* currentProcess=getHighestPriorityUnblocked(&quantum);
    if (currentProcess==NULL)
        return;

    int lowerBound=currentProcess->memoryBoundaries[0];

    word* instruction = Memory[currentProcess->pc+currentProcess->memoryBoundaries[0]+9];
    printf("Executing Instruction: %s\n", instruction->value);
    int blocking = whichBlocking(instruction);
    if (blocking!=0){
        if (blocking<0){ // semSignal{
            mutexes[-blocking-1]=0;
            PCB* dequeud=dequeueHighestBlockedPriorityOfResource(-blocking-1);  // dequeue the highest priority PCB from the blocked mutexes queue
            enqueue(dequeud); // enqueue the dequeued PCB to the ready queue
            dequeueBlocked(dequeud); // dequeue the PCB from the general blocked queue and set its state to ready i.e 0
        }
        else{ // semWait
            if (!mutexes[blocking-1])  // mutex is available
                flipMutex(blocking-1); // lock the mutex
            else{ // mutex is not available
                enqueueBlocked(currentProcess);
                enqueueBlockedResource(currentProcess, blocking-1);
                dequeue(currentProcess);
                currentProcess->state = 1;
            }
        }
    }
    else
        executeInstruction(currentProcess, instruction);

    incrementPC(currentProcess);

}


int main()
{
    int arrivals[3] = {1, 2, 3};
    // for (int clock = 0; clock < 1e3; clock++)
    // {
    //     for (int i = 0; i < 3; i++)
    //     {
    //         if (arrivals[i] == clock)
    //         {
    //             PCB* process = (PCB*)malloc(sizeof(PCB));
    //             switch (i)
    //             {
    //             case 0:
    //                 process = read("Program_1.txt");
    //                 break;
    //             case 1:
    //                 process = read("Program_2.txt");
    //                 break;
    //             case 2:
    //                 process = read("Program_3.txt");
    //                 break;
    //             default:
    //                 break;
    //             }
    //             enqueue(process);
    //         }
    //     }
    //     execute();
    // }




    PCB* process=read("Program_1.txt");
    // printf("PID: %d\n", process->PID);

    enqueue(process);



    // int i = 0;
    // while (Memory[i] != NULL)
    // {
    //     printMemory(i);
    //     i++;
    // }


    // for (int i = 0; i < 4; i++)
    // {
    //     printQueue(queues[i]);
    // }

    // int* quantum;
    // printf("highest Unblocked PID: %d\n", getHighestPriorityUnblocked(quantum)->PID);
    // printf("highest PID quantum: %d\n",*quantum);
    execute();
    execute();
    execute();
    execute();
    execute();
    execute();
    for (int i = 0; i < 20; i++)
        if (Memory[i] != NULL)
            printMemory(i);
    printf("pc: %d\n", process->pc);
    printf("Mutex: %d %d %d\n", mutexes[0],mutexes[1],mutexes[2]);
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








//