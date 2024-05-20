#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>




char* readFile(char* filename){
    FILE *file = fopen(filename, "r");
    char line[100];
    char* content = "";
    while (fgets(line, 300, file) != NULL){
        strcat(content, line);
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
        char* content = readFile(filename);
        newWord->value = strdup(content);
    }
    else if (strcmp(thirdArg,"input")==0){
        char* input = malloc(100);
        printf("Enter value for %s: ", varToBeAssigned);
        scanf("%s", input);
        newWord->value = strdup(input);
    }
    else
        newWord->value = strdup(thirdArg);


    if (location<0)
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
    char* filename = strtok(strdup(instruction->value), " ");
    char* content = strtok(NULL, " ");
    FILE *file = fopen(filename, "w");
    fprintf(file, "%s", content);
    fclose(file);
}
void printFromTo(PCB* pcb,word* instruction){
    char* op=strtok(strdup(instruction->value)," ");
    char* varToBePrinted=strtok(NULL," ");
    char* from=strtok(NULL," ");
    char* to=strtok(NULL," ");
    int location=findVariable(pcb,varToBePrinted);
    if (location<0)
        printf("NO variable with name %s\n", varToBePrinted);
    else{
        for (int i = atoi(from); i <= atoi(to); i++)
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