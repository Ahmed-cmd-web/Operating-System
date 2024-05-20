#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>



typedef struct {
    int PID;
    bool state;  // blocked/unblocked   True->blocked, False->unblocked
    int priority;
    int pc;
    int memoryBoundaries[2];   // [startInMemory, endInMemory]
} PCB;

typedef struct {
    char *name;
    char *value;
} word;