#include "cmnd.h"
#include "Scanf.h"
#include "print.h"
#include "str.h"

void prototype_cmnd() {
    char* input = "";
    scanf("{s}", input);
    command cmnd;
    parse_cmnd(input, &cmnd);
    print("You entered command: {s}\n", cmnd.name);
    print("Command data: {s}\n", cmnd.data);
    execute_cmnd(cmnd);
}

void parse_cmnd(char* input, command* cmnd){
    int i = 0, x = 0;
    for (; input[i] != ' ' && input[i] != '\0'; i++){
        cmnd->name[i] = input[i];
    }
    cmnd->name[i] = '\0';
    if(input[i] == ' ' && input[i+1] != '\0')
    {
        i++;
        for (; input[i] != '\0'; x++,i++)
        {
            cmnd->data[x] = input[i];
        }
        cmnd->data[x] = '\0';
    }
    else {
        cmnd->data[0] = '\0';
    }
    
}

void execute_cmnd(command cmnd){
    if (strcmp(cmnd.name, "exit")){
        print("Exiting PotetOS...\n");
    }
}
