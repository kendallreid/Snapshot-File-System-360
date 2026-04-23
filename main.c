#include <stdio.h>
#include <string.h>
#include "vcs/vcs.h"

#include "filesystem/file_system.h"
#include "command_handlers.h"

#define MAX_INPUT 128
FileSystem fs;

int main()
{

    char input[MAX_INPUT];
    char cmd[32];
    char arg[96];

    fs_init(&fs);

    // initialize working directory
    fs.cwd_inode = 0;

    printf("Simple VC Snapshot File System\n");
    printf("Commands:\n");
    printf("\tcreate <path>\n");
    printf("\tls <path>\n");
    printf("\twrite <path>\n");
    printf("\tread <path>\n");
    printf("\tdelete <path>\n");
    printf("\tlookup <path>\n");
    printf("\tcd <path>\n");
    printf("\tinit\n");
    printf("\tcommit <message>\n");
    printf("\tlog\n");
    printf("\tcheckout <commit-id>\n");
    printf("\texit\n\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = 0;
        arg[0] = '\0';

        if (sscanf(input, "%31s %95[^\n]", cmd, arg) < 1)
        {
            printf("invalid input\n");
            continue;
        }

        switch (get_command_id(cmd))
        {
            case CMD_CREATE:
                handle_create(&fs, arg);
                break;

            case CMD_WRITE:
                handle_write(&fs, arg);
                break;

            case CMD_READ:
                handle_read(&fs, arg);
                break;

            case CMD_LS:
                fs_ls(&fs, arg[0] ? arg : "");
                break;

            case CMD_DELETE:
                handle_delete(&fs, arg);
                break;

            case CMD_LOOKUP:
            {
                int inode = fs_lookup_path(&fs, arg);
                if (inode == -1)
                    printf("not found\n");
                else
                    printf("inode: %d\n", inode);
                break;
            }

            case CMD_CD:
                handle_cd(&fs, arg);
                break;

            case CMD_EXIT:
                return 0;

            default:
                if (strcmp(cmd, "init") == 0)
                {
                    if (vcs_init() != 0)
                        printf("vcs init failed\n");
                }
                else if (strcmp(cmd, "commit") == 0)
                {
                    if (!arg[0])
                        printf("commit requires a message\n");
                    else if (vcs_commit(arg) != 0)
                        printf("commit failed\n");
                }
                else if (strcmp(cmd, "log") == 0)
                {
                    if (vcs_log() != 0)
                        printf("log failed\n");
                }
                else if (strcmp(cmd, "checkout") == 0)
                {
                    if (!arg[0])
                        printf("checkout requires a commit id\n");
                    else if (vcs_checkout(arg) != 0)
                        printf("checkout failed\n");
                }
                else
                {
                    printf("unknown command\n");
                }
        }
    }

    return 0;
}