#include "command_handlers.h"

#include <string.h>

#include "filesystem/file_system.h"
#include "shared_values.h"

// User input determines command for switch statment
int get_command_id(const char *cmd)
{
    if (strcmp(cmd, "create") == 0) return CMD_CREATE;
    if (strcmp(cmd, "write") == 0)  return CMD_WRITE;
    if (strcmp(cmd, "read") == 0)   return CMD_READ;
    if (strcmp(cmd, "ls") == 0)     return CMD_LS;
    if (strcmp(cmd, "delete") == 0) return CMD_DELETE;
    if (strcmp(cmd, "lookup") == 0) return CMD_LOOKUP;
    if (strcmp(cmd, "exit") == 0)   return CMD_EXIT;

    return CMD_UNKNOWN;
}

// Creates file/directory
void handle_create()
{
    char name[64];
    char type[16];

    printf("filename: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    printf("type (file/dir): ");
    fgets(type, sizeof(type), stdin);
    type[strcspn(type, "\n")] = 0;

    inode_type t = (strcmp(type, "dir") == 0) ? DIR_TYPE : FILE_TYPE;

    if (fs_create(name, t) == -1)
        printf("create failed\n");
}

// Writes to specified file
void handle_write()
{
    char name[64];

    printf("filename: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    int inode = fs_lookup(name);
    if (inode == -1)
    {
        printf("file not found\n");
        return;
    }

    // collect multi-line input
    printf("Enter text (end with '.' on a line):\n");

    char data[4096];
    char line[256];

    data[0] = '\0';

    while (1)
    {
        fgets(line, sizeof(line), stdin);
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, ".") == 0)
            break;

        strcat(data, line);
        strcat(data, "\n");
    }

    if (fs_write(inode, data, strlen(data)) == -1)
        printf("write failed\n");
}

// Read a file
void handle_read()
{
    char name[64];

    printf("filename: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    int inode = fs_lookup(name);
    if (inode == -1)
    {
        printf("file not found\n");
        return;
    }

    int file_size = fs_get_size(inode);
    if (file_size < 0)
    {
        printf("invalid file\n");
        return;
    }

    char buffer[file_size + 1];  // allocate exact size (+1 for '\0')

    int read_size = fs_read(inode, buffer, file_size);
    if (read_size < 0)
    {
        printf("read error\n");
        return;
    }

    buffer[read_size] = '\0';

    printf("%s\n", buffer);
}

// Delete a file or directory
void handle_delete()
{
    char name[64];

    printf("name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    if (fs_delete(name) == -1)
        printf("delete failed\n");
}

// Lookup an entry's inode
void handle_lookup()
{
    char name[64];

    printf("name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    int inode = fs_lookup(name);

    if (inode == -1)
        printf("not found\n");
    else
        printf("inode: %d\n", inode);
}