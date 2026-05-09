#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "utils.c"
#include "operations.c"

int linear = 0;
int currentClock = 0;

PCB* getHighestPriorityUnblocked(int* quantum);

PCB* read(char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL)
        return NULL;

    /* Count instructions (one line = one word in memory after header). */
    int burstTime = 0;
    char line[100];
    while (fgets(line, 100, file) != NULL)
        burstTime++;

    int sizeWords = 9 + burstTime;
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    int start = allocateProcessMemory(sizeWords, NULL);
    if (start < 0) {
        fclose(file);
        return NULL;
    }

    PCB* newPCB = (PCB*)malloc(sizeof(PCB));
    if (newPCB == NULL) {
        fclose(file);
        return NULL;
    }

    newPCB->PID = currentPID++;
    newPCB->arrivalTime = currentClock;
    newPCB->burstTime = burstTime;
    newPCB->state = 0;
    newPCB->priority = 0;
    newPCB->pc = 0;
    newPCB->memoryBoundaries[0] = start;
    newPCB->memoryBoundaries[1] = start + sizeWords;
    newPCB->inMemory = true;
    newPCB->sizeWords = sizeWords;

    Memory[start] = createPCBattr("PID", newPCB->PID);
    Memory[start + 1] = createPCBattr("State", newPCB->state);
    Memory[start + 2] = createPCBattr("Priority", newPCB->priority);
    Memory[start + 3] = createPCBattr("PC", newPCB->pc);
    Memory[start + 4] = createPCBattr("lowerBound", newPCB->memoryBoundaries[0]);
    Memory[start + 5] = createPCBattr("upperBound", newPCB->memoryBoundaries[1]);
    for (int r = 6; r <= 8; r++)
        Memory[start + r] = NULL;

    int ins = 0;
    while (fgets(line, 100, file) != NULL) {
        word* newWord = (word*)malloc(sizeof(word));
        line[strcspn(line, "\r\n")] = '\0';
        newWord->name = "Instruction";
        newWord->value = strdup(line);
        Memory[start + 9 + ins++] = newWord;
    }

    fclose(file);
    recomputeLastMemoryPosition();
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



void execute(int schedulerType){
    int quantum;
    PCB* currentProcess = NULL;

    if (schedulerType == 1) {
        currentProcess = getHRRNProcess();
        quantum = 1000;
    }
    else if (schedulerType == 2) {
        currentProcess = getHighestPriorityUnblocked(&quantum);
        if(currentProcess != NULL){
            quantum = 2;
        }
    }
    else {
        currentProcess = getHighestPriorityUnblocked(&quantum);
        quantum = 1 << (quantum - 1);
    }

    if (currentProcess==NULL){
        return;
    }

    ensureInMemory(currentProcess);
    if (!currentProcess->inMemory) {
        fprintf(stderr, "Could not load process %d into memory (swap/OOM)\n", currentProcess->PID);
        return;
    }

    printf("\nChosen Process: %d\n", currentProcess->PID);
    printAllQueues();



    int removed = 0;
    for (int i = 0; i < quantum; i++)
    {
        int insIndex = currentProcess->pc + currentProcess->memoryBoundaries[0] + 9;
        if (!isValidProcessMemoryIndex(currentProcess, insIndex)) {
            printf("PC out of process memory for PID %d, stopping.\n", currentProcess->PID);
            break;
        }
        word *instruction = Memory[insIndex];
        if (instruction == NULL) {
            printf("No instruction at PC for process %d; stopping.\n", currentProcess->PID);
            break;
        }
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
                    ensureInMemory(dequeud);
                    enqueue(dequeud);        // enqueue the dequeued PCB to the ready queue
                    dequeueBlocked(dequeud); // dequeue the PCB from the general blocked queue and set its state to ready i.e 0
                    printf("Unblocking Process: %d\n", dequeud->PID);
                    printf("Process %d is now using resource %d\n", dequeud->PID, -blocking);
                    printAllQueues();
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
                    printAllQueues();
                    removed = 1;
                    break;
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
            printAllQueues();
            break;
        }


    }
    if (!removed)
    {
        if (schedulerType == 2)   // RR
        {
            dequeue(currentProcess);
            enqueue(currentProcess);
        }
        else if (schedulerType == 3)   // MLFQ
        {
            if(currentProcess->priority < 3){
                degradePriority(currentProcess);
            }
            else{
                dequeue(currentProcess);
                enqueue(currentProcess);
            }
        }
    }
}


int main()
{

    int arrivals[3];
    int schedulerType;
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

    printf("\nChoose Scheduler:\n");
    printf("1 = HRRN\n");
    printf("2 = Round Robin\n");
    printf("3 = MLFQ\n");
    scanf("%d", &schedulerType);

    if(schedulerType == 3)
    {
        printf("Enter 1 for linear execution, 0 for exponential quantum: ");
        scanf("%d",&linear);
    }

    printf("==============================================\n");
    printf("Note: Resources are hashed according to the following:\n");
    printf("1->file resource \n");
    printf("2->userInput resource \n");
    printf("3->userOutput resource \n");
    printf("==============================================\n");

    for (currentClock = 0; currentClock < 1000; currentClock++)
    {
        for (int i = 0; i < 3; i++)
        {
            if (arrivals[i] == currentClock)
            {
                PCB* process;

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
                }
                if(process != NULL && schedulerType == 2)
                    process->priority = 0;


                if(process != NULL)
                    enqueue(process);
            }
        }

        printf("\nCLOCK CYCLE = %d\n", currentClock);

        execute(schedulerType);

        printAllMemory();

        if(currentClock >= 4)
        {
            int q;
            if(getHighestPriorityUnblocked(&q) == NULL && getHRRNProcess() == NULL)
            {
                break;
            }
        }
    }

    return 0;
}
