#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

#include <stdio.h>
#include <stdint.h>

#include "filesystem/file_system.h"

#define CMD_CREATE  1
#define CMD_WRITE   2
#define CMD_READ    3
#define CMD_LS      4
#define CMD_DELETE  5
#define CMD_LOOKUP  6
#define CMD_CD      7
#define CMD_EXIT    8
#define CMD_UNKNOWN 0

int get_command_id(const char *cmd);

void handle_create(FileSystem *fs, const char *path);
void handle_write(FileSystem *fs, const char *path);
void handle_read(FileSystem *fs, const char *path);
void handle_delete(FileSystem *fs, const char *path);
void handle_lookup(FileSystem *fs, const char *path);
void handle_cd(FileSystem *fs, const char *path);

#endif