#ifndef CMND_H
#define CMND_H

typedef struct command {
    char name[32];
    char data[256];
}command;

void prototype_cmnd();
void parse_cmnd(char* input, command* cmnd);
void execute_cmnd(command cmnd);


#endif