#include "data.c"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>




int findVariable(PCB* pcb, char* varName){
    for (int i = pcb->memoryBoundaries[0]; i < pcb->memoryBoundaries[1]; i++)
        if (Memory[i] !=NULL&& strcmp(Memory[i]->name, varName) == 0)
            return i;

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
    Memory[findVariable(pcb, "PC")]->value = strdup(buf);
}

void enqueue(PCB* pcb){
    int i = 0;
    while (queues[pcb->priority][i] != NULL)
        i++;
    queues[pcb->priority][i] = pcb;
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
void printMemory(int i){
    printf("%d- ", i);
    word *word = Memory[i];
    printf("name: %s  ", word->name);
    printf("value:%s \n", word->value);
}