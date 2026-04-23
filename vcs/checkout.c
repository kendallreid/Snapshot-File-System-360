#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "vcs.h"
#include "helpers.h"
#include "block_operations.h"
#include "../shared_values.h"

#define VCS_MAX_PATH 256
#define VCS_MAX_ENTRIES 256
#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(DirEntry))

typedef struct
{
    char name[MAX_NAME_LENGTH];
    int inode_index;
    inode_type type;
} VcsEntry;

// Lists the contents of a directory inode into the provided output array.
// Returns the number of entries copied, or -1 on error.
static int list_dir(int dir_ino, VcsEntry *out, int max)
{
    // Reject invalid inode indices.
    if (dir_ino < 0 || dir_ino >= MAX_INODES)
    {
        return -1;
    }

    // Reject unused inodes.
    if (!fs.inode_table[dir_ino].used)
    {
        return -1;
    }

    // Reject non-directory inodes.
    if (fs.inode_table[dir_ino].type != DIR_TYPE)
    {
        return -1;
    }

    Inode *dir = &fs.inode_table[dir_ino];
    int count = 0;

    // Walk through every data block owned by the directory.
    for (int b = 0; b < dir->block_count; b++)
    {
        uint8_t block[BLOCK_SIZE];

        // Read the current directory block from disk.
        if (read_block(fs.disk, dir->blocks[b], block) != 0)
        {
            return -1;
        }

        DirEntry *entries = (DirEntry *)block;

        // Scan every directory entry stored in this block.
        for (int i = 0; i < ENTRIES_PER_BLOCK; i++)
        {
            // Skip empty directory slots.
            if (entries[i].name[0] == '\0')
            {
                continue;
            }

            // Stop if the caller's output buffer is full.
            if (count >= max)
            {
                return count;
            }

            // Copy entry metadata into the caller's array.
            strncpy(out[count].name, entries[i].name, MAX_NAME_LENGTH - 1);
            out[count].name[MAX_NAME_LENGTH - 1] = '\0';
            out[count].inode_index = entries[i].inode_index;
            out[count].type = fs.inode_table[entries[i].inode_index].type;
            count++;
        }
    }

    return count;
}

// Copies a single file from src to dst inside the virtual filesystem.
// Returns 0 or a write result on success, or -1 on failure.
static int copy_file(const char *src, const char *dst)
{
    // Resolve the source path to an inode.
    int src_ino = fs_lookup_path(&fs, src);
    if (src_ino < 0)
    {
        return -1;
    }

    // Read the source file size.
    int size = fs_get_size(&fs, src_ino);
    if (size < 0)
    {
        return -1;
    }

    void *buf = NULL;

    // Allocate a buffer and read the file if it is not empty.
    if (size > 0)
    {
        buf = malloc((size_t)size);
        if (!buf)
        {
            return -1;
        }

        if (fs_read(&fs, src_ino, buf, size) != size)
        {
            free(buf);
            return -1;
        }
    }

    // Make sure the destination file exists.
    int dst_ino = vcs_ensure_exists(dst, FILE_TYPE);
    if (dst_ino < 0)
    {
        free(buf);
        return -1;
    }

    // Write the source contents into the destination file.
    int rc = fs_write(&fs, dst_ino, buf, size);
    free(buf);

    return rc;
}

// Recursively copies a directory tree from src_dir to dst_dir.
// Returns 0 on success, or -1 on failure.
static int copy_tree(const char *src_dir, const char *dst_dir)
{
    // Resolve the source directory.
    int src_ino = fs_lookup_path(&fs, src_dir);
    if (src_ino < 0)
    {
        return -1;
    }

    // Ensure the destination directory exists.
    if (vcs_ensure_exists(dst_dir, DIR_TYPE) < 0)
    {
        return -1;
    }

    VcsEntry entries[VCS_MAX_ENTRIES];

    // List entries in the source directory.
    int count = list_dir(src_ino, entries, VCS_MAX_ENTRIES);
    if (count < 0)
    {
        return -1;
    }

    // Copy each entry one by one.
    for (int i = 0; i < count; i++)
    {
        char src[VCS_MAX_PATH];
        char dst[VCS_MAX_PATH];

        // Build source and destination child paths.
        if (vcs_path_join(src, sizeof(src), src_dir, entries[i].name) != 0)
        {
            return -1;
        }

        if (vcs_path_join(dst, sizeof(dst), dst_dir, entries[i].name) != 0)
        {
            return -1;
        }

        // Recurse into directories, copy regular files directly.
        if (entries[i].type == DIR_TYPE)
        {
            if (copy_tree(src, dst) != 0)
            {
                return -1;
            }
        }
        else
        {
            if (copy_file(src, dst) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

// Recursively deletes a file or directory tree at the given path.
// Returns 0 on success, or -1 on failure.
static int delete_tree(const char *path)
{
    // Resolve the target path.
    int ino = fs_lookup_path(&fs, path);
    if (ino < 0)
    {
        return -1;
    }

    // If it is a file, delete it directly.
    if (fs.inode_table[ino].type == FILE_TYPE)
    {
        return fs_delete(&fs, path);
    }

    VcsEntry entries[VCS_MAX_ENTRIES];

    // Otherwise list the directory contents first.
    int count = list_dir(ino, entries, VCS_MAX_ENTRIES);
    if (count < 0)
    {
        return -1;
    }

    // Recursively delete every child entry.
    for (int i = 0; i < count; i++)
    {
        char child[VCS_MAX_PATH];

        if (vcs_path_join(child, sizeof(child), path, entries[i].name) != 0)
        {
            return -1;
        }

        if (delete_tree(child) != 0)
        {
            return -1;
        }
    }

    // Delete the now-empty directory itself.
    return fs_delete(&fs, path);
}

// Removes everything from the root directory except the .vcs metadata directory.
// Returns 0 on success, or -1 on failure.
static int clear_root_except_vcs(void)
{
    VcsEntry entries[VCS_MAX_ENTRIES];

    // List all entries in the root directory.
    int count = list_dir(0, entries, VCS_MAX_ENTRIES);
    if (count < 0)
    {
        return -1;
    }

    // Delete every root entry except .vcs.
    for (int i = 0; i < count; i++)
    {
        if (strcmp(entries[i].name, ".vcs") == 0)
        {
            continue;
        }

        char child[VCS_MAX_PATH];

        if (vcs_path_join(child, sizeof(child), "/", entries[i].name) != 0)
        {
            return -1;
        }

        if (delete_tree(child) != 0)
        {
            return -1;
        }
    }

    return 0;
}

// Updates the HEAD file to point at the given commit id.
// Returns 0 on success, or -1 on failure.
static int set_head(const char *id)
{
    return vcs_write_text("/.vcs/HEAD", id);
}

// Checks out a commit by replacing the working tree with the saved commit tree.
// Returns 0 on success, or -1 on failure.
int vcs_checkout(const char *commit_id)
{
    char tree_dir[VCS_MAX_PATH];

    // Build the path to the commit's stored tree.
    snprintf(tree_dir, sizeof(tree_dir), "/.vcs/commits/%s/tree", commit_id);

    // Fail if the commit does not exist.
    if (fs_lookup_path(&fs, tree_dir) < 0)
    {
        fprintf(stderr, "unknown commit: %s\n", commit_id);
        return -1;
    }

    // Clear the current working tree, restore the target tree, and update HEAD.
    if (clear_root_except_vcs() != 0)
    {
        return -1;
    }

    if (copy_tree(tree_dir, "/") != 0)
    {
        return -1;
    }

    if (set_head(commit_id) != 0)
    {
        return -1;
    }

    printf("checked out %s\n", commit_id);
    return 0;
}