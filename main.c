#include <stdio.h>
#include <string.h>

#include "filesystem/file_system.h"
#include "command_handlers.h"

#define MAX_INPUT 128

int main()
{
    FileSystem fs;
    char input[MAX_INPUT];
    char cmd[32];

    fs_init(&fs);

    printf("Simple Snapshot File System\n");
    printf("Commands:\n\t"
           "create\n\twrite\n\tread\n\tls\n\tdelete\n\tlookup\n\texit\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        // Read full input line
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = 0;

        // Extract command safely
        if (sscanf(input, "%31s", cmd) != 1)
        {
            printf("invalid input\n");
            continue;
        }

        switch (get_command_id(cmd))
        {
            case CMD_CREATE:
                handle_create(&fs);
                break;

            case CMD_WRITE:
                handle_write(&fs);
                break;

            case CMD_READ:
                handle_read(&fs);
                break;

            case CMD_LS:
                fs_ls(&fs);
                break;

            case CMD_DELETE:
                handle_delete(&fs);
                break;

            case CMD_LOOKUP:
                handle_lookup(&fs);
                break;

            case CMD_EXIT:
                return 0;

            default:
                printf("unknown command\n");
        }
    }

    return 0;
}