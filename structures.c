#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>



typedef struct {
    int PID;
    bool state;  // blocked/unblocked   True->blocked, False->unblocked
    int priority;
    int pc;
    int memoryBoundaries[2];   // [startInMemory, endInMemory] end exclusive
    int arrivalTime;
    int burstTime;
    bool inMemory;             // false when swapped to disk, still in scheduler
    int sizeWords;            // 9 + burstTime (one contiguous region in Memory[40])
} PCB;

typedef struct {
    char *name;
    char *value;
} word;