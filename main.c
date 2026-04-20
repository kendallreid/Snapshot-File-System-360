#include <stdio.h>
#include <string.h>
#include "filesystem/file_system.h"
#include "command_handlers.h"

#define MAX_INPUT 128


int main()
{
    char input[MAX_INPUT];
    char cmd[32];

    fs_init();

    printf("Simple Snapshot File System\n");
    printf("Commands:\n\t"
           "create\n\twrite\n\tread\n\tls\n\tdelete\n\tlookup\n\texit\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        // Input too large
        if (!fgets(input, sizeof(input), stdin))
            break;
        input[strcspn(input, "\n")] = 0;

        sscanf(input, "%s", cmd);

        // Interactive mode, could be more command like if needed
        switch (get_command_id(cmd))
        {
            case CMD_CREATE:
                handle_create();
                break;

            case CMD_WRITE:
                handle_write();
                break;

            case CMD_READ:
                handle_read();
                break;

            case CMD_LS:
                fs_ls();
                break;

            case CMD_DELETE:
                handle_delete();
                break;

            case CMD_LOOKUP:
                handle_lookup();
                break;

            case CMD_EXIT:
                return 0;

            default:
                printf("unknown command\n");
        }
    }
    return 0;
}