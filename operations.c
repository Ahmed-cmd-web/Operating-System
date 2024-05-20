#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

char *readFile(char *filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    // Initialize an empty string with dynamic memory allocation
    size_t capacity = 1024; // Initial capacity
    size_t length = 0;
    char* content = malloc(capacity);
    if (content == NULL)
    {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }
    content[0] = '\0'; // Start with an empty string

    char line[100];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        size_t line_length = strlen(line);

        // Ensure there's enough space to concatenate the new line
        if (length + line_length + 1 > capacity)
        {
            capacity = (capacity + line_length) * 2;
            char *new_content = realloc(content, capacity);
            if (new_content == NULL)
            {
                perror("Error reallocating memory");
                free(content);
                fclose(file);
                return NULL;
            }
            content = new_content;
        }

        // Concatenate the new line
        strcat(content, line);
        length += line_length;
    }

    fclose(file);
    return content;
}

void assign(PCB *pcb, word *instruction){
    char* name = instruction->name;
    char* value = strdup(instruction->value);
    char* operation = strtok(value, " ");
    char* varToBeAssigned = strtok(NULL, " ");
    char* thirdArg = strtok(NULL, " ");
    int location=findVariable(pcb,varToBeAssigned);
    word* newWord = (word*)malloc(sizeof(word));
    newWord->name = varToBeAssigned;
    if (strcmp(thirdArg,"readFile")==0){
        char* filename = strtok(NULL, " ");
        int actualLocation = findVariable(pcb, filename);
        char* content = readFile(Memory[actualLocation]->value);
        if (content != NULL)
            newWord->value = strdup(content);
        else
            return;
    }
    else if (strcmp(thirdArg,"input")==0){
        char* input = malloc(100);
        printf("Enter value for %s: ", varToBeAssigned);
        scanf("%s", input);
        newWord->value = strdup(input);
    }
    else
        newWord->value = strdup(thirdArg);

    if (location < 0)
        Memory[-location] = newWord;
    else
        Memory[location] = newWord;

}

void print(PCB* pcb,word* instruction){
    char* op=strtok(strdup(instruction->value)," ");
    char* varToBePrinted=strtok(NULL," ");
    int location=findVariable(pcb,varToBePrinted);
    if (location<0)
        printf("NO variable with name %s\n", varToBePrinted);
    else
        printf("%s\n", Memory[location]->value);
}
void writeFile(PCB* pcb,word* instruction){
    char* operation = strtok(strdup(instruction->value), " ");
    char* filename = strtok(NULL, " ");
    char* content = strtok(NULL, " ");

    FILE *file = fopen(Memory[findVariable(pcb,filename)]->value, "w");
    fprintf(file, "%s", Memory[findVariable(pcb,content)]->value);
    fclose(file);
}
void printFromTo(PCB* pcb,word* instruction){
    char* op=strtok(strdup(instruction->value)," ");
    char* from=strtok(NULL," ");
    char* to=strtok(NULL," ");
    int var1=findVariable(pcb,from);
    int var2=findVariable(pcb,to);

    if (var1<0)
        printf("NO variable with name %d\n", var1);
    else if (var2<0)
        printf("NO variable with name %d\n", var2);
    else{
        for (int i = atoi(Memory[var1]->value); i <= atoi(Memory[var2]->value); i++)
            printf("%d ", i);
        printf("\n");
    }
}


void executeInstruction(PCB* pcb, word* instruction){
    char* operation=strtok(strdup(instruction->value), " ");
    if (strcmp(operation, "assign") == 0)
        assign(pcb, instruction);
    else if (strcmp(operation, "print") == 0)
        print(pcb, instruction);
    else if (strcmp(operation, "writeFile") == 0)
        writeFile(pcb, instruction);
    else if (strcmp(operation, "printFromTo") == 0)
        printFromTo(pcb, instruction);

}