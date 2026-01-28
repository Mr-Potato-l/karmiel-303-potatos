#include "cmnd.h"
#include "Scanf.h"
#include "print.h"
#include "str.h"

void prototype_cmnd() {
    char* cmnd = "";
    scanf("{s}", cmnd);
    print("You entered command: {s}\n", cmnd);
    struct cmnd command;
    command.name = cmnd;
    command.data = "";
    execute_cmnd(command);
}

void execute_cmnd(struct cmnd command){
    if (strcmp(command.name, "exit")){
        print("Exiting PotetOS...\n");
    }
}
