#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "structures.c"


word* Memory[60];
PCB* generalBlockedQueue[3];  // 1 queue for the 3 resources {Accessing file,Taking user input,Outputting }   True->blocked, False->unblocked
PCB* queues[4][3];  // 4 queues for the 4 priorities
PCB* blockedResources[3][3];  // 3 queues for the 3 resources {Accessing file,Taking user input,Outputting }
bool mutexes[3];  // 3 mutexes for the 3 resources {0->Accessing file,Taking user input,Outputting }   True->blocked, False->unblocked
int currentPID = 0;
int lastMemoryPosition = 0;