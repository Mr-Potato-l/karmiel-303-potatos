#include "cmnd.h"
#include "Scanf.h"
#include "print.h"

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
    if (command.name == "exit"){
        print("Exiting PotetOS...\n");
        while (1) {
            __asm__ __volatile__("hlt");
        }
    }
}
