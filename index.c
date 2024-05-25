#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "utils.c"
#include "operations.c"

int linear = 0;

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

    // Memory[previousMemoryPosition++] = NULL;
    // Memory[previousMemoryPosition++] = NULL;
    // Memory[previousMemoryPosition++] = NULL;
    lastMemoryPosition+=3;


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
    if (!linear)
        quantum = 1 << (quantum-1);
    if (currentProcess==NULL)
        return;
    int lowerBound=currentProcess->memoryBoundaries[0];
    int removed = 0;
    for (int i = 0; i < quantum; i++)
    {
        word *instruction = Memory[currentProcess->pc + currentProcess->memoryBoundaries[0] + 9];
        printf("Executing Process(quantum left=%d): %d\n", quantum-i,currentProcess->PID);

        printf("Executing Instruction: %s\n", instruction->value);

        int blocking = whichBlocking(instruction);
        if (blocking != 0)
        {
            if (blocking < 0)
            { // semSignal
                mutexes[-blocking - 1] = 0;
                PCB *dequeud = dequeueHighestBlockedPriorityOfResource(-blocking - 1); // dequeue the highest priority PCB from the blocked mutexes queue
                if (dequeud != NULL)
                {
                    enqueue(dequeud);        // enqueue the dequeued PCB to the ready queue
                    dequeueBlocked(dequeud); // dequeue the PCB from the general blocked queue and set its state to ready i.e 0
                    printf("Unblocking Process: %d\n", dequeud->PID);
                    printf("Process %d is now using resource %d\n", dequeud->PID, -blocking);
                }
            }
            else
            { // semWait
                if (!mutexes[blocking - 1]){  // mutex is available
                    flipMutex(blocking - 1); // lock the mutex
                    printf("Process %d is now using resource %d\n", currentProcess->PID, blocking);
                }
                else
                { // mutex is not available
                    printf("Blocking Process because resource not available: %d\n", currentProcess->PID);
                    currentProcess->state = 1;
                    enqueueBlocked(currentProcess);
                    enqueueBlockedResource(currentProcess, blocking - 1);
                    dequeue(currentProcess);
                }
            }
        }
        else
            executeInstruction(currentProcess, instruction);




        incrementPC(currentProcess);
        if (removeIfDone(currentProcess))
        {
            removed = 1;
            printf("Process %d is done\n", currentProcess->PID);
            break;
        }


    }
    if (!removed)
        degradePriority(currentProcess);


}


int main()
{

    int arrivals[3];
    char *input = malloc(100);
    printf("Enter arrival time for %s: ","process 1" );
    scanf("%s", input);
    arrivals[0] = atoi(input);
    printf("Enter arrival time for %s: ","process 2" );
    scanf("%s", input);
    arrivals[1] = atoi(input);
    printf("Enter arrival time for %s: ","process 3" );
    scanf("%s", input);
    arrivals[2] = atoi(input);

    printf("Enter 1 for linear execution, 0 for quantum execution(exponential): ");
    scanf("%d", &linear);

    printf("==============================================\n");
    printf("Note: Resources are hashed according to the following:\n");
    printf("1->file resource \n");
    printf("2->userInput resource \n");
    printf("3->userOutput resource \n");
    printf("==============================================\n");

    for (int clock = 0; clock < 1e3; clock++)
    {
        for (int i = 0; i < 3; i++)
        {
            if (arrivals[i] == clock)
            {
                PCB* process = (PCB*)malloc(sizeof(PCB));
                switch (i)
                {
                case 0:
                    process = read("Program_1.txt");
                    break;
                case 1:
                    process = read("Program_2.txt");
                    break;
                case 2:
                    process = read("Program_3.txt");
                    break;
                default:
                    break;
                }
                enqueue(process);
            }
        }
        execute();
    }

    return 0;
}
