cmake_minimum_required(VERSION 3.28)
project(Snapshot_File_System_360 C)

set(CMAKE_C_STANDARD 11)

include_directories(filestructures)
include_directories(filesystem)
include_directories(vcs)

add_executable(Snapshot_File_System_360
        main.c
        command_handlers.c
        filesystem/file_system.c
        filestructures/bitmap.c
        filestructures/block_operations.c
        vcs/checkout.c
        vcs/commit.c
        vcs/helpers.c
        vcs/init.c
        vcs/log.c
)