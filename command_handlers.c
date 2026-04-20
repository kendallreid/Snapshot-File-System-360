#include "command_handlers.h"

#include <string.h>
#include <stdio.h>
#include "shared_values.h"

// Handle converting typed command text into command IDs
int get_command_id(const char *cmd)
{
    // Match command names
    if (strcmp(cmd, "create") == 0) return CMD_CREATE;
    if (strcmp(cmd, "write") == 0)  return CMD_WRITE;
    if (strcmp(cmd, "read") == 0)   return CMD_READ;
    if (strcmp(cmd, "ls") == 0)     return CMD_LS;
    if (strcmp(cmd, "delete") == 0) return CMD_DELETE;
    if (strcmp(cmd, "lookup") == 0) return CMD_LOOKUP;
    if (strcmp(cmd, "cd") == 0)     return CMD_CD;
    if (strcmp(cmd, "exit") == 0)   return CMD_EXIT;

    // No match found
    return CMD_UNKNOWN;
}

// Handle create command
void handle_create(FileSystem *fs, const char *path)
{
    // Require a path
    if (!path || strlen(path) == 0)
    {
        printf("invalid path\n");
        return;
    }

    char type[16];

    // Ask user what to create
    printf("type (file/dir): ");
    if (!fgets(type, sizeof(type), stdin)) return;

    // Remove newline
    type[strcspn(type, "\n")] = 0;

    inode_type t;

    // Convert text to enum type
    if (strcmp(type, "file") == 0)
        t = FILE_TYPE;
    else if (strcmp(type, "dir") == 0)
        t = DIR_TYPE;
    else
    {
        printf("invalid type\n");
        return;
    }

    // Create object in file system
    if (fs_create(fs, path, t) == -1)
        printf("create failed\n");
}

// Handle write command
void handle_write(FileSystem *fs, const char *path)
{
    // Require a path
    if (!path || strlen(path) == 0)
    {
        printf("invalid path\n");
        return;
    }

    // Find file
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
    {
        printf("file not found\n");
        return;
    }

    printf("Enter text (end with '.' on a line):\n");

    char data[4096];
    char line[256];

    // Start empty text buffer
    data[0] = '\0';

    while (1)
    {
        // Read one line
        if (!fgets(line, sizeof(line), stdin)) break;

        // Remove newline
        line[strcspn(line, "\n")] = 0;

        // Stop on period
        if (strcmp(line, ".") == 0)
            break;

        // Append line to full text
        strcat(data, line);
        strcat(data, "\n");
    }

    // Write data to file
    if (fs_write(fs, inode, data, strlen(data)) == -1)
        printf("write failed\n");
}

// Handle read command
void handle_read(FileSystem *fs, const char *path)
{
    // Require a path
    if (!path || strlen(path) == 0)
    {
        printf("invalid path\n");
        return;
    }

    // Find file
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
    {
        printf("file not found\n");
        return;
    }

    // Get file size
    int file_size = fs_get_size(fs, inode);

    if (file_size < 0)
    {
        printf("invalid file\n");
        return;
    }

    // Allocate read buffer
    char buffer[file_size + 1];

    // Read contents
    int read_size = fs_read(fs, inode, buffer, file_size);

    if (read_size < 0)
    {
        printf("read error\n");
        return;
    }

    // End string and print
    buffer[read_size] = '\0';
    printf("%s\n", buffer);
}

// Handle delete command
void handle_delete(FileSystem *fs, const char *path)
{
    // Require a path
    if (!path || strlen(path) == 0)
    {
        printf("invalid path\n");
        return;
    }

    // Find target
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
    {
        printf("not found\n");
        return;
    }

    // Protect root folder
    if (inode == 0)
    {
        printf("cannot delete root\n");
        return;
    }

    // Delete object
    if (fs_delete(fs, path) == -1)
        printf("delete failed\n");
}

// Handle lookup command
void handle_lookup(FileSystem *fs, const char *path)
{
    // Require a path
    if (!path || strlen(path) == 0)
    {
        printf("invalid path\n");
        return;
    }

    // Resolve inode number
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
        printf("not found\n");
    else
        printf("inode: %d\n", inode);
}

// Handle change directory command
void handle_cd(FileSystem *fs, const char *path)
{
    // Find directory
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
    {
        printf("path not found\n");
        return;
    }

    // Must be a directory
    if (fs->inode_table[inode].type != DIR_TYPE)
    {
        printf("not a directory\n");
        return;
    }

    // Update working directory
    fs->cwd_inode = inode;

    printf("changed directory\n");
}