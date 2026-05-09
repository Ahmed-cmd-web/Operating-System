#include "data.c"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/stat.h> // Linux/Mac

extern int currentClock;

/* --- Memory index bounds (per-process protect) --- */
int isValidProcessMemoryIndex(PCB* pcb, int index)
{
    if (index < 0 || index >= 40)
        return 0;
    if (index < pcb->memoryBoundaries[0] || index >= pcb->memoryBoundaries[1]) {
        fprintf(stderr, "Memory access violation: PID %d, index %d not in [%d, %d)\n",
            pcb->PID, index, pcb->memoryBoundaries[0], pcb->memoryBoundaries[1]);
        return 0;
    }
    return 1;
}

/* --- Recompute high-water mark of used slots --- */
void recomputeLastMemoryPosition(void)
{
    int m = 0;
    for (int i = 0; i < 40; i++) {
        if (Memory[i] != NULL)
            m = i + 1;
    }
    lastMemoryPosition = m;
}

/* --- Contiguous free runs --- */
int largestFreeRun(int* startOut)
{
    int best = 0;
    int bestStart = 0;
    int cur = 0;
    int curStart = 0;
    for (int i = 0; i < 40; i++) {
        if (Memory[i] == NULL) {
            if (cur == 0)
                curStart = i;
            cur++;
        } else {
            if (cur > best) {
                best = cur;
                bestStart = curStart;
            }
            cur = 0;
        }
    }
    if (cur > best) {
        best = cur;
        bestStart = curStart;
    }
    if (startOut)
        *startOut = bestStart;
    return best;
}

int findFreeBlock(int size)
{
    if (size <= 0 || size > 40)
        return -1;
    int run = 0;
    for (int i = 0; i < 40; i++) {
        if (Memory[i] == NULL) {
            run++;
            if (run == size)
                return i - size + 1;
        } else
            run = 0;
    }
    return -1;
}

static void collectUniqueInMemoryPcb(PCB* list[64], int* n, PCB* p)
{
    if (p == NULL)
        return;
    for (int k = 0; k < *n; k++) {
        if (list[k] != NULL && list[k]->PID == p->PID)
            return;
    }
    list[(*n)++] = p;
}

static int pcb_cmp_by_low(const void* a, const void* b)
{
    PCB* const* pa = (PCB* const*)a;
    PCB* const* pb = (PCB* const*)b;
    int la = (*pa)->memoryBoundaries[0];
    int lb = (*pb)->memoryBoundaries[0];
    if (la < lb) return -1;
    if (la > lb) return 1;
    return 0;
}

/* Collect all resident (inMemory) PCBs for compaction / victim scan */
int collectInMemoryPCBs(PCB* out[64])
{
    int n = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++)
            if (queues[i][j] != NULL && queues[i][j]->inMemory)
                collectUniqueInMemoryPcb(out, &n, queues[i][j]);
    }
    for (int i = 0; i < 3; i++) {
        if (generalBlockedQueue[i] != NULL && generalBlockedQueue[i]->inMemory)
            collectUniqueInMemoryPcb(out, &n, generalBlockedQueue[i]);
    }
    for (int r = 0; r < 3; r++)
        for (int i = 0; i < 3; i++)
            if (blockedResources[r][i] != NULL && blockedResources[r][i]->inMemory)
                collectUniqueInMemoryPcb(out, &n, blockedResources[r][i]);
    return n;
}

/*
 * Select largest in-memory process; tie: highest priority value (0..3) = lowest importance.
 * Never returns exclude.
 */
PCB* pickLargestInMemoryVictim(PCB* exclude)
{
    int n;
    PCB* all[64];
    n = collectInMemoryPCBs(all);
    PCB* best = NULL;
    for (int i = 0; i < n; i++) {
        PCB* c = all[i];
        if (c == NULL || c == exclude || !c->inMemory)
            continue;
        if (best == NULL) {
            best = c;
            continue;
        }
        if (c->sizeWords > best->sizeWords)
            best = c;
        else if (c->sizeWords == best->sizeWords) {
            if (c->priority > best->priority) /* same size: evict lower priority (higher number) */
                best = c;
        }
    }
    return best;
}

static void getSwapPath(int pid, char* buf, size_t buflen)
{
    snprintf(buf, buflen, "./disk/swap_%d.txt", pid);
}

int compactMemory(void)
{
    int n;
    PCB* procs[64];
    n = collectInMemoryPCBs(procs);
    if (n == 0)
        return 0;
    if (n > 1)
        qsort(procs, n, sizeof(PCB*), pcb_cmp_by_low);

    int dest = 0;
    for (int p = 0; p < n; p++) {
        PCB* pcb = procs[p];
        if (pcb == NULL)
            continue;
        int L = pcb->memoryBoundaries[0];
        int H = pcb->memoryBoundaries[1];
        int size = H - L;
        if (size <= 0)
            continue;
        if (L != dest) {
            for (int j = size - 1; j >= 0; j--) {
                Memory[dest + j] = Memory[L + j];
                if (L + j != dest + j)
                    Memory[L + j] = NULL;
            }
            pcb->memoryBoundaries[0] = dest;
            pcb->memoryBoundaries[1] = dest + size;
            int o4 = dest + 4;
            int o5 = dest + 5;
            if (Memory[o4] && Memory[o4]->name && strcmp(Memory[o4]->name, "lowerBound") == 0) {
                char s[20];
                snprintf(s, sizeof(s), "%d", dest);
                if (Memory[o4]->value) free(Memory[o4]->value);
                Memory[o4]->value = strdup(s);
            }
            if (Memory[o5] && Memory[o5]->name && strcmp(Memory[o5]->name, "upperBound") == 0) {
                char s[20];
                snprintf(s, sizeof(s), "%d", dest + size);
                if (Memory[o5]->value) free(Memory[o5]->value);
                Memory[o5]->value = strdup(s);
            }
        }
        dest = pcb->memoryBoundaries[1];
    }
    recomputeLastMemoryPosition();
    return 1;
}

void swapOut(PCB *v)
{
    if (v == NULL || !v->inMemory)
        return;

    char path[256]; // Larger buffer for full path
    getSwapPath(v->PID, path, sizeof(path));

    fprintf(stderr, "DEBUG: Writing to: %s\n", path);

    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        perror("swapOut fopen");
        return;
    }

    int L = v->memoryBoundaries[0];
    int H = v->memoryBoundaries[1];
    for (int i = L; i < H; i++)
    {
        if (Memory[i] == NULL)
        {
            fprintf(f, "__EMPTY__\n");
        }
        else
        {
            char *nm = Memory[i]->name != NULL ? Memory[i]->name : "";
            char *val = Memory[i]->value != NULL ? Memory[i]->value : "";
            fprintf(f, "%s|", nm);
            fputs(val, f);
            fputc('\n', f);
        }
    }
    fclose(f);

    // Clear memory
    for (int i = L; i < H; i++)
    {
        if (Memory[i] != NULL)
        {
            if (Memory[i]->value)
                free(Memory[i]->value);
        }
        Memory[i] = NULL;
    }
    v->inMemory = false;
    recomputeLastMemoryPosition();
}

int allocateProcessMemory(int size, PCB* requester)
{
    if (size <= 0 || size > 40)
        return -1;
    for (int round = 0; round < 64; round++) {
        int start = findFreeBlock(size);
        if (start != -1) {
            if (start + size > lastMemoryPosition)
                lastMemoryPosition = start + size;
            return start;
        }
        /* Merge holes: slide resident processes down. */
        compactMemory();
        start = findFreeBlock(size);
        if (start != -1) {
            if (start + size > lastMemoryPosition)
                lastMemoryPosition = start + size;
            return start;
        }
        PCB* v = pickLargestInMemoryVictim(requester);
        if (v != NULL) {
            swapOut(v);
            continue;
        }
        return -1; /* OOM: cannot fit and nothing left to evict (or only requester) */
    }
    return -1;
}

/* Parse a line: name|rest as value; special __EMPTY__ */
void swapIn(PCB* p)
{
    if (p->inMemory)
        return;
    char path[64];
    getSwapPath(p->PID, path, sizeof(path));
    FILE* f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "swapIn: missing swap file %s\n", path);
        return;
    }
    int start = allocateProcessMemory(p->sizeWords, p);
    if (start < 0) {
        fprintf(stderr, "swapIn: OOM for PID %d (need %d words)\n", p->PID, p->sizeWords);
        fclose(f);
        return;
    }
    char line[2048];
    for (int i = 0; i < p->sizeWords; i++) {
        if (fgets(line, sizeof(line), f) == NULL) {
            fprintf(stderr, "swapIn: short file for PID %d at slot %d\n", p->PID, i);
            for (; i < p->sizeWords; i++)
                Memory[start + i] = NULL;
            break;
        }
        char* nl = strpbrk(line, "\r\n");
        if (nl) *nl = '\0';
        if (strcmp(line, "__EMPTY__") == 0) {
            Memory[start + i] = NULL;
            continue;
        }
        char* sep = strchr(line, '|');
        if (sep == NULL) {
            Memory[start + i] = NULL;
            continue;
        }
        *sep = '\0';
        char* name = line;
        char* value = sep + 1;
        word* w = (word*)malloc(sizeof(word));
        w->name = name[0] ? strdup(name) : NULL;
        w->value = value[0] ? strdup(value) : NULL;
        Memory[start + i] = w;
    }
    fclose(f);
    if (remove(path) != 0) {
        perror("swapIn remove");
    }
    p->memoryBoundaries[0] = start;
    p->memoryBoundaries[1] = start + p->sizeWords;
    p->inMemory = true;
    int o4 = start + 4;
    int o5 = start + 5;
    if (Memory[o4] && Memory[o4]->name && (strcmp(Memory[o4]->name, "lowerBound") == 0)) {
        char s[20];
        snprintf(s, sizeof(s), "%d", start);
        if (Memory[o4]->value) free(Memory[o4]->value);
        Memory[o4]->value = strdup(s);
    }
    if (Memory[o5] && Memory[o5]->name && (strcmp(Memory[o5]->name, "upperBound") == 0)) {
        char s[20];
        snprintf(s, sizeof(s), "%d", start + p->sizeWords);
        if (Memory[o5]->value) free(Memory[o5]->value);
        Memory[o5]->value = strdup(s);
    }
    recomputeLastMemoryPosition();
}

void ensureInMemory(PCB* p)
{
    if (p != NULL && !p->inMemory)
        swapIn(p);
}

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

int findVariable(PCB* pcb, char* varName)
{
    if (pcb == NULL)
        return -100;
    if (!pcb->inMemory) {
        fprintf(stderr, "findVariable: PID %d is not in memory; ensureInMemory() first\n", pcb->PID);
        return -100;
    }
    for (int i = pcb->memoryBoundaries[0]; i < pcb->memoryBoundaries[1] && i < 40; i++) {
        if (Memory[i] != NULL && Memory[i]->name != NULL && strcmp(Memory[i]->name, varName) == 0)
            return i;
    }
    /* Three reserved variable cells at +6, +7, +8 (only) */
    for (int k = 0; k < 3; k++) {
        int i = pcb->memoryBoundaries[0] + 6 + k;
        if (i >= pcb->memoryBoundaries[1] || i < 0 || i >= 40)
            break;
        if (!isValidProcessMemoryIndex(pcb, i))
            return -100;
        if (Memory[i] == NULL)
            return -i;
    }
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



int removeIfDone(PCB* pcb)
{
    if (pcb == NULL)
        return 0;
    if (!pcb->inMemory)
        return 0;
    if ((pcb->pc + 9 + pcb->memoryBoundaries[0]) >= pcb->memoryBoundaries[1]) {
        int L = pcb->memoryBoundaries[0];
        int H = pcb->memoryBoundaries[1];
        for (int i = L; i < H; i++) {
            if (Memory[i] != NULL) {
                if (Memory[i]->value != NULL)
                    free(Memory[i]->value);
                /* names are usually string literals; variables may share pointers */
            }
            Memory[i] = NULL;
        }
        pcb->inMemory = false;
        char path[64];
        snprintf(path, sizeof(path), "swap_%d.txt", pcb->PID);
        (void)remove(path);

        recomputeLastMemoryPosition();
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
    printf("v                        v\n");
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


PCB* getHRRNProcess()
{
    PCB* best = NULL;
    double bestRatio = -1;

    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            PCB* p = queues[i][j];

            if(p != NULL && p->state == 0)
            {
                int waiting = currentClock - p->arrivalTime;
                int remaining = p->burstTime - p->pc;

                if(remaining <= 0)
                    remaining = 1;

                double ratio =
                ((double)(waiting + remaining)) / remaining;

                if(ratio > bestRatio)
                {
                    bestRatio = ratio;
                    best = p;
                }
            }
        }
    }

    return best;
}


void printAllQueues()
{
    printf("\n=========== READY QUEUES ===========\n");

    for(int i = 0; i < 4; i++)
    {
        printf("Queue %d:\n", i);
        printQueue(queues[i]);
    }

    printf("\n=========== BLOCKED QUEUE ==========\n");
    printQueue(generalBlockedQueue);

    printf("====================================\n");
}

void printAllMemory()
{
    printf("\n============= MEMORY =============\n");

    for(int i = 0; i < 40; i++)
    {
        if(Memory[i] != NULL)
            printMemory(i);
        else
            printf("%d- EMPTY\n", i);
    }

    printf("==================================\n");
}
