#include "cmnd.h"
#include "Scanf.h"
#include "print.h"
#include "str.h"
#include "fs_api.h"

void ls(command cmnd);
void mkdir(command cmnd);
void rmdir(command cmnd);
void mkf(command cmnd);
void rmf(command cmnd);
void echo(command cmnd);
void cd (command cmnd);

void cmnd_input() {
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
    else if(strcmp(cmnd.name, "mkf")) {
        mkf(cmnd);
    }
    else if(strcmp(cmnd.name, "rmf")) {
        rmf(cmnd);
    }
    else if(strcmp(cmnd.name, "help")){
        print("Available commands:\n");
        print("help - Show this help message\n");
        print("ls - List files and directories\n");
        print("mkdir - Create a new directory\n");
        print("rmdir - Removes a directory\n");
        print("mkf - Create a new file\n");
        print("rmf - Remove a file\n");
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
        result = fs_api_listdir(fs_api_getcwd(), entries, MAX_DIR_ENTRIES); // List root directory, since it's the current one.
    } else {
        result = fs_api_listdir(cmnd.data, entries, MAX_DIR_ENTRIES); // List specified directory
    }

    if (result >= 0) {
        for (uint32_t i = 0; i < result && entries[i].name[0] != '\0'; i++) {
            char type_char = 'f';  /* default to file */
            if (entries[i].type == 1) {
                type_char = 'd';   /* directory */
            } else if (entries[i].type == 2) {
                type_char = 'c';   /* device */
            }
            print("{c} {s}\n", type_char, entries[i].name);
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

void mkf(command cmnd) {
    if (cmnd.data[0] == '\0') {
        print("mkf requires a filename\n");
    } else {
        int32_t handle = fs_api_open(cmnd.data, FS_MODE_CREATE | FS_MODE_WRITE);
        if (handle < 0) {
            print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(handle));
        } else {
            fs_api_close(handle);
        }
    }
}

void rmf(command cmnd) {
    int32_t result = fs_api_remove(cmnd.data);
    if(result != FS_OK) {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }
}

// primitive echo command, without file reading
void echo(command cmnd){
    int32_t result = 0;
    bool file_exists = false;
    result = fs_api_listdir(fs_api_getcwd(), entries, MAX_DIR_ENTRIES); // List root directory, since it's the current one.
    if (result >= 0) {
        for (uint32_t i = 0; i < result && entries[i].name[0] != '\0'; i++) {
            if (strcmp(cmnd.data, entries[i].name)){
                print("file exists. here is the content:\n");
                file_exists = true;
            }
        }
    }
    else {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }

    if (file_exists) {
        char buffer[256];
        int32_t read_result = fs_api_readfile(cmnd.data, buffer, sizeof(buffer));
        if (read_result >= 0) {
            buffer[read_result] = '\0'; // Null-terminate the content
            print("{s}\n", buffer);
        } else {
            print("Error reading file '{s}': {s}\n", cmnd.data, fs_api_strerror(read_result));
        }
    } 
    else {
        print("{s}\n", cmnd.data);
    }
}

void cd (command cmnd){
    print("directory before is: {s}\n", fs_api_getcwd());
    int32_t result = fs_api_chdir(cmnd.data);
    if(result != FS_OK) {
        print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
    }
    else {
        print("directory now is: {s}\n", fs_api_getcwd());
    }
}
