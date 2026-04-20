#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

#include <stdio.h>
#include <stdint.h>

// command IDs
#define CMD_CREATE  1
#define CMD_WRITE   2
#define CMD_READ    3
#define CMD_LS      4
#define CMD_DELETE  5
#define CMD_LOOKUP  6
#define CMD_EXIT    7
#define CMD_UNKNOWN 0

int get_command_id(const char *cmd);
void handle_create();
void handle_write();
void handle_read();
void handle_delete();
void handle_lookup();

#endif