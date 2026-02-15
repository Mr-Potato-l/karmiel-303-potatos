#include "cmnd.h"
#include "Scanf.h"
#include "print.h"
#include "str.h"
#include "fs_api.h"

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
        fs_dirent_t entries[MAX_DIR_ENTRIES];
        int32_t result;

        if (strcmp(cmnd.data, "\0") != 0) {
            result = fs_api_listdir("/", entries, MAX_DIR_ENTRIES); // List root directory, since it's the current one.
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
    else if(strcmp(cmnd.name, "mkdir")) {
        int32_t result = fs_api_mkdir(cmnd.data);
        if(result != FS_OK) {
            print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
        }
    }
    else if(strcmp(cmnd.name, "rmdir")) {
        int32_t result = fs_api_rmdir(cmnd.data);
        if(result != FS_OK) {
            print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
        }
    }
    else if(strcmp(cmnd.name, "mkf")) {
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
    else if(strcmp(cmnd.name, "rmf")) {
        int32_t result = fs_api_remove(cmnd.data);
        if(result != FS_OK) {
            print("'{s}' - {s}\n", cmnd.data, fs_api_strerror(result));
        }
    }
    else if(strcmp(cmnd.name, "help")){
        print("Available commands:\n");
        print("help - Show this help message\n");
        print("ls - List files and directories\n");
        print("mkdir - Create a new directory\n");
        print("rmdir - Removes a directory\n");
        print("mkf - Create a new file\n");
        print("rmf - Remove a file\n");
        print("exit - Exit the operating system\n");
    }
}
