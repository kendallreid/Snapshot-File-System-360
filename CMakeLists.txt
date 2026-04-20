cmake_minimum_required(VERSION 3.28)
project(Snapshot_File_System_360 C)

set(CMAKE_C_STANDARD 11)

include_directories(filestructures)
include_directories(filesystem)

add_executable(Snapshot_File_System_360
        filestructures/bitmap.c
        filestructures/bitmap.h
        filestructures/Block.h
        filestructures/block_operations.c
        filestructures/block_operations.h
        shared_values.h
        filestructures/testbitmap.c
        filesystem/inode.h)
