#include "data.c"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

void printMemory(int i){
    printf("%d- ", i);
    word *word = Memory[i];
    printf("name: %s  ", word->name);
    printf("value:%s \n", word->value);
}
void dequeue(PCB* pcb){
    for (int i = 0; i < 3; i++)
        if (queues[pcb->priority][i] !=NULL&& queues[pcb->priority][i]->PID == pcb->PID)
        {
            queues[pcb->priority][i] = NULL;
            return;
        }
}

int findVariable(PCB* pcb, char* varName){
    for (int i = pcb->memoryBoundaries[0]; i < pcb->memoryBoundaries[1]; i++){
        if (Memory[i] != NULL && strcmp(Memory[i]->name, varName) == 0)
            return i;
        }
    printf("NOT found variable with name %s\n", varName);


    for (int i = pcb->memoryBoundaries[0]+6; i < pcb->memoryBoundaries[0]+10; i++)
        if (Memory[i]==NULL)
            return -i;
    printf("NOT  enough space for variable with name %s\n", varName);
    return -100;
}

int whichResource(char* resource){
    if (strcmp(resource,"file")==0)
        return 1;
    else if (strcmp(resource,"userInput")==0)
        return 2;
    else if (strcmp(resource,"userOutput")==0)
        return 3;
    return 0;
}


int removeIfDone(PCB* pcb){
    if ((pcb->pc+9+pcb->memoryBoundaries[0]) >= pcb->memoryBoundaries[1]){
        for (int i = pcb->memoryBoundaries[0]; i < pcb->memoryBoundaries[1]; i++)
            Memory[i] = NULL;
        for (int i = 0; i < 3; i++)
            if (queues[pcb->priority][i] != NULL && queues[pcb->priority][i]->PID == pcb->PID){
                queues[pcb->priority][i] = NULL;
                return 1;
            }
    }
    return 0;
}

void enqueue(PCB* pcb){
    int i = 0;
    while (queues[pcb->priority][i] != NULL)
        i++;
    queues[pcb->priority][i] = pcb;
}

void degradePriority(PCB* pcb){
    if (pcb->priority < 3){
        dequeue(pcb);
        pcb->priority++;
        char buf [20];
        snprintf(buf, 10, "%d", pcb->priority);
        Memory[findVariable(pcb, "Priority")]->value = strdup(buf);
        enqueue(pcb);
    }
}

int whichBlocking(word* instruction){
    char* operation=strtok(strdup(instruction->value), " ");
    char* resource=strtok(NULL, " ");
    if (strcmp(operation,"semWait")==0)
        return whichResource(resource);
    else if (strcmp(operation,"semSignal")==0)
        return -whichResource(resource);
    return 0;
}



void incrementPC(PCB* pcb){
    pcb->pc++;
    char buf [20];
    snprintf(buf, 10, "%d", pcb->pc);
    int loc = findVariable(pcb, "PC");
    Memory[loc]->value = strdup(buf);
}


void enqueueBlocked(PCB* pcb){
    int i = 0;
    while (generalBlockedQueue[i] != NULL)
        i++;
    generalBlockedQueue[i] = pcb;
}
void enqueueBlockedResource(PCB* pcb, int resource){
    int i = 0;
    while (blockedResources[resource][i] != NULL)
        i++;
    blockedResources[resource][i] = pcb;
}



PCB* dequeueHighestBlockedPriorityOfResource(int resource){
    int currentPriority = 100;
    PCB* highestPriorityPCB=NULL;
    for (int i = 0; i < 3; i++)
        if (blockedResources[resource][i] != NULL){
            if (blockedResources[resource][i]->priority < currentPriority){
                highestPriorityPCB = blockedResources[resource][i];
                currentPriority = highestPriorityPCB->priority;
            }
        }
    return highestPriorityPCB;
}

void dequeueBlocked(PCB* pcb){
    for (int i = 0; i < 3; i++)
        if (generalBlockedQueue[i]!=NULL  &&  (generalBlockedQueue[i]->PID) == (pcb->PID)){
            generalBlockedQueue[i] = NULL;
            pcb->state = 0;
            return;
        }
}


void flipMutex(int resource){
    mutexes[resource] = !mutexes[resource];
}
void printQueue(PCB* queue[]){
    printf("Start                   END\n");
    printf("⬇️                          ⬇️\n");
    printf("----------------------------\n");
    for (int i = 0; i < 3; i++)
        if (queue[i] != NULL)
            printf("%d           ", queue[i]->PID);
        else
            printf("_            ");

    printf("\n----------------------------\n");
}


word* createPCBattr(char* name, int value){
    word* newWord = (word*)malloc(sizeof(word)); // Allocate memory dynamically
    if (newWord == NULL) {
        // Handle memory allocation failure
        return NULL;
    }
    newWord->name = name;
    char buf [20];
    snprintf(buf, 10, "%d", value);
    newWord->value = strdup(buf); // Allocate memory for the string and copy the value
    return newWord;
}
