# Linux-Based Snapshot File System with Git-Like Versioning
##  By: Jace Dunn, Andrea Luquin, Kendall Reid

## Project Overview

This project is a simplified file system written in C that runs entirely in user space on top of a virtual disk image. The system recreates core operating system concepts such as inodes, directories, block storage, and file read/write operations. It also adds Git-like snapshot functionality, allowing previous filesystem states to be saved and restored using a copy-on-write model.

The project demonstrates four major course themes:

- File system implementation  
- System-level I/O  
- Computer memory systems  
- Control flow  

---

## Core Features

- Disk formatting (`mkfs`)
- Inode-based file and directory storage
- Directory hierarchy and path traversal
- File creation, deletion, reading, and writing
- Current working directory support
- Directory listing (`ls`)
- Persistent storage across mounts
- Snapshots, history, and restore functionality

---

## Filesystem Architecture

```text
Virtual Disk File
├── Metadata / Reserved Blocks
├── Bitmap
├── Inode Table
└── Data Blocks
```

## Inodes

Each file or directory has an inode storing:

- type (file or directory)
- file size
- allocated blocks
- parent inode
- usage state

## Directories

Directories store name-to-inode mappings.


## FileSystem Implementation

- Inode table loading and saving
- Path resolution (`/a/b/file.txt`)
- Directory entry lookup and insertion
- File and directory creation
- Directory listing (`ls`)
- Current directory navigation (`cd`)
- File deletion
- File read/write support

Key Focus Areas

- Organizing files and folders internally
- Converting paths into inode references
- Managing parent/child directory relationships

## Design Decisions and Tradeoffs

Inode-Based Design - Used inodes to separate metadata from filenames, similar to Unix systems.

Fixed-Size Tables - Simpler implementation, but limits maximum number of files.

Linear Directory Search - Easy to implement, but slower for large directories.

User-Space Filesystem - Safer and easier to debug than a kernel implementation.

Copy-on-Write Snapshots - Efficient versioning, but adds metadata complexity.

## Course Themes Demonstrated

- File System Implementation - Directories, inodes, blocks, persistence.

- System-Level I/O - Managing the virtual disk through low-level file operations.

- Computer Memory Systems - Struct layouts, buffers, bitmaps, block organization.

- Control Flow - Traversal logic, command handling, allocation decisions.
