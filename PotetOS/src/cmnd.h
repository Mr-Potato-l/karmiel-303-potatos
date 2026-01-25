#ifndef CMND_H
#define CMND_H

struct cmnd {
    char* name;
    char* data;
};

void prototype_cmnd();
void parse_cmnd(char* input, struct cmnd* command);
void execute_cmnd(struct cmnd command);


#endif