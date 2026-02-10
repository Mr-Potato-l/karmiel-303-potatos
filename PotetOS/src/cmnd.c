#include "cmnd.h"
#include "Scanf.h"
#include "print.h"
#include "str.h"
#include "fs_api.h"

void ls(command cmnd);
void mkdir(command cmnd);
void rmdir(command cmnd);
void echo(command cmnd);
void cd (command cmnd);

void prototype_cmnd() {
    char* input = "";
    scanf("{s}", input);
    command cmnd;
    parse_cmnd(input, &cmnd);
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
    else if(strcmp(cmnd.name, "ls")) {
        ls(cmnd);
    }
    else if(strcmp(cmnd.name, "mkdir")) {
        mkdir(cmnd);
    }
    else if(strcmp(cmnd.name, "rmdir")) {
        rmdir(cmnd);
    }
    else if(strcmp(cmnd.name, "echo")) {
        echo(cmnd);
    }
    else if (strcmp(cmnd.name, "cd")) {
        cd(cmnd);
    }
    else if(strcmp(cmnd.name, "help")){
        print("Available commands:\n");
        print("help - Show this help message\n");
        print("ls - List files and directories\n");
        print("mkdir - Create a new directory\n");
        print("rmdir - Removes a directory\n");
        print("echo - Print the provided text\n");
        print("exit - Exit the operating system\n");
    }
    else {
        print("Unknown command. try using the 'help' command\n");
    }
}

void ls(command cmnd){
    fs_dirent_t entries[MAX_DIR_ENTRIES];
    int32_t result;

    if (strcmp(cmnd.data, "\0") != 0) {
        result = fs_api_listdir("/", entries, MAX_DIR_ENTRIES); // List root directory, since it's the current one.
    } else {
        result = fs_api_listdir(cmnd.data, entries, MAX_DIR_ENTRIES); // List specified directory
    }

    if (result >= 0) {
        for (uint32_t i = 0; i < result && entries[i].name[0] != '\0'; i++) {
            print("- {s}\n", entries[i].name);
        }
    } else {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }
}

void mkdir(command cmnd){
    int32_t result = fs_api_mkdir(cmnd.data);
    if(result != FS_OK) {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }
}

void rmdir(command cmnd){
    int32_t result = fs_api_rmdir(cmnd.data);
    if(result != FS_OK) {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }
}

// primitive echo command, without file reading
void echo(command cmnd){
    print("{s}\n", cmnd.data);
}

void cd (command cmnd){
    print("current directory is: {s}\n", fs_api_getcwd());
    int32_t result = fs_api_chdir(cmnd.data);
    if(result != FS_OK) {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }
    else {
        print("current directory is: {s}\n", fs_api_getcwd());
    }
}
