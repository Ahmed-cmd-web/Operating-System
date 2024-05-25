#include "data.c"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int comp(const void *elem1, const void *elem2)
{
    PCB *f = ((PCB *)elem1);
    PCB *s = ((PCB *)elem2);
    if (f->priority > s->priority)
        return 1;
    if (f->priority < s->priority)
        return -1;
    return 0;
}
int sort(PCB *arr[])
{
    qsort(arr, sizeof(arr) / sizeof(PCB), sizeof(PCB), comp);

    return 0;
}

void printMemory(int i){
    printf("%d- ", i);
    word *word = Memory[i];
    printf("name: %s  ", word->name);
    printf("value:%s \n", word->value);
}
void dequeue(PCB* pcb){
    for (int i = 0; i < 3; i++)
        if (queues[pcb->priority][i] !=NULL&& queues[pcb->priority][i]->PID == pcb->PID)
            queues[pcb->priority][i] = NULL;
    // shift forward
    for (int i = 0; i < 3; i++)
        if (queues[pcb->priority][i] == NULL){
            for (int j = i+1; j < 3; j++)
                queues[pcb->priority][j-1] = queues[pcb->priority][j];
            queues[pcb->priority][2] = NULL;
            return;
        }
    sort(queues[pcb->priority]);
}

int findVariable(PCB* pcb, char* varName){
    for (int i = pcb->memoryBoundaries[0]; i < pcb->memoryBoundaries[1]; i++){
        if (Memory[i] != NULL && strcmp(Memory[i]->name, varName) == 0)
            return i;
        }

    for (int i = pcb->memoryBoundaries[0]+6; i < pcb->memoryBoundaries[0]+10; i++)
        if (Memory[i]==NULL)
            return -i;
    printf("NOT  enough space for variable with name %s\n", varName);
    return -100;
}

int hashResource(char* resource){
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
        dequeue(pcb);
        return 1;
    }
    return 0;
}

void enqueue(PCB* pcb){
    int i = 0;
    while (queues[pcb->priority][i] != NULL)
        i++;
    queues[pcb->priority][i] = pcb;
    sort(queues[pcb->priority]);
}

void degradePriority(PCB* pcb){
    if (pcb->priority < 3){
        dequeue(pcb);
        pcb->priority++;
        char buf [20];
        snprintf(buf, 10, "%d", pcb->priority);
        Memory[findVariable(pcb, "Priority")]->value = strdup(buf);
        enqueue(pcb);
        sort(queues[pcb->priority]);
    }
}

int whichBlocking(word* instruction){
    char* operation=strtok(strdup(instruction->value), " ");
    char* resource=strtok(NULL, " ");
    if (strcmp(operation,"semWait")==0)
        return hashResource(resource);
    else if (strcmp(operation,"semSignal")==0)
        return -hashResource(resource);
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
    sort(generalBlockedQueue);
}
void enqueueBlockedResource(PCB* pcb, int resource){
    int i = 0;
    while (blockedResources[resource][i] != NULL)
        i++;
    blockedResources[resource][i] = pcb;
    sort(blockedResources[resource]);
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

    // remove the highest priority PCB from the blocked queue
    for (int i = 0; i < 3; i++)
        if (blockedResources[resource][i] != NULL  &&  (blockedResources[resource][i]->PID) == (highestPriorityPCB->PID))
            blockedResources[resource][i] = NULL;


    // shift forward
    for (int i = 0; i < 3; i++)
        if (blockedResources[resource][i] == NULL){
            for (int j = i+1; j < 3; j++)
                blockedResources[resource][j-1] = blockedResources[resource][j];
            blockedResources[resource][2] = NULL;
            return highestPriorityPCB;
        }


    return highestPriorityPCB;
}

void dequeueBlocked(PCB* pcb){
    for (int i = 0; i < 3; i++)
        if (generalBlockedQueue[i]!=NULL  &&  (generalBlockedQueue[i]->PID) == (pcb->PID)){
            generalBlockedQueue[i] = NULL;
            pcb->state = 0;
        }

    // shift forward
    for (int i = 0; i < 3; i++)
        if (generalBlockedQueue[i] == NULL){
            for (int j = i+1; j < 3; j++)
                generalBlockedQueue[j-1] = generalBlockedQueue[j];
            generalBlockedQueue[2] = NULL;
            return;
        }
    sort(generalBlockedQueue);
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
