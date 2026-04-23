#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "vcs.h"
#include "helpers.h"
#include "block_operations.h"
#include "../shared_values.h"

#define VCS_MAX_PATH 256
#define VCS_MAX_TEXT 1024
#define VCS_MAX_ENTRIES 256
#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(DirEntry))

typedef struct
{
    char name[MAX_NAME_LENGTH];
    int inode_index;
    inode_type type;
} VcsEntry;

// Lists directory contents into the output array.
// Returns number of entries, or -1 on failure.
static int list_dir(int dir_ino, VcsEntry *out, int max)
{
    // Validate inode index and type.
    if (dir_ino < 0 || dir_ino >= MAX_INODES)
    {
        return -1;
    }

    if (!fs.inode_table[dir_ino].used)
    {
        return -1;
    }

    if (fs.inode_table[dir_ino].type != DIR_TYPE)
    {
        return -1;
    }

    Inode *dir = &fs.inode_table[dir_ino];
    int count = 0;

    // Iterate over all blocks in the directory.
    for (int b = 0; b < dir->block_count; b++)
    {
        uint8_t block[BLOCK_SIZE];

        // Read block from disk.
        if (read_block(fs.disk, dir->blocks[b], block) != 0)
        {
            return -1;
        }

        DirEntry *entries = (DirEntry *)block;

        // Iterate through entries in block.
        for (int i = 0; i < ENTRIES_PER_BLOCK; i++)
        {
            // Skip empty entries.
            if (entries[i].name[0] == '\0')
            {
                continue;
            }

            // Stop if output buffer is full.
            if (count >= max)
            {
                return count;
            }

            // Copy entry data.
            strncpy(out[count].name, entries[i].name, MAX_NAME_LENGTH - 1);
            out[count].name[MAX_NAME_LENGTH - 1] = '\0';
            out[count].inode_index = entries[i].inode_index;
            out[count].type = fs.inode_table[entries[i].inode_index].type;
            count++;
        }
    }

    return count;
}

// Copies a file from src to dst.
// Returns write result or -1 on failure.
static int copy_file(const char *src, const char *dst)
{
    // Resolve source inode.
    int src_ino = fs_lookup_path(&fs, src);
    if (src_ino < 0)
    {
        return -1;
    }

    // Get file size.
    int size = fs_get_size(&fs, src_ino);
    if (size < 0)
    {
        return -1;
    }

    void *buf = NULL;

    // Read file contents if non empty.
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

    // Ensure destination file exists.
    int dst_ino = vcs_ensure_exists(dst, FILE_TYPE);
    if (dst_ino < 0)
    {
        free(buf);
        return -1;
    }

    // Write data to destination.
    int rc = fs_write(&fs, dst_ino, buf, size);
    free(buf);

    return rc;
}

// Recursively copies a directory tree.
static int copy_tree(const char *src_dir, const char *dst_dir)
{
    // Resolve source directory.
    int src_ino = fs_lookup_path(&fs, src_dir);
    if (src_ino < 0)
    {
        return -1;
    }

    // Ensure destination directory exists.
    if (vcs_ensure_exists(dst_dir, DIR_TYPE) < 0)
    {
        return -1;
    }

    VcsEntry entries[VCS_MAX_ENTRIES];

    // List entries in source.
    int count = list_dir(src_ino, entries, VCS_MAX_ENTRIES);
    if (count < 0)
    {
        return -1;
    }

    // Copy each entry.
    for (int i = 0; i < count; i++)
    {
        char src[VCS_MAX_PATH];
        char dst[VCS_MAX_PATH];

        // Build paths.
        if (vcs_path_join(src, sizeof(src), src_dir, entries[i].name) != 0)
        {
            return -1;
        }

        if (vcs_path_join(dst, sizeof(dst), dst_dir, entries[i].name) != 0)
        {
            return -1;
        }

        // Recurse or copy file.
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

// Reads the next commit id from disk.
// Returns id or -1 on failure.
static int get_next_id(void)
{
    char buf[64];

    if (vcs_read_text("/.vcs/NEXT", buf, sizeof(buf)) != 0)
    {
        return -1;
    }

    int n = atoi(buf);

    return n > 0 ? n : 1;
}

// Writes the next commit id.
static int set_next_id(int n)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "%d", n);

    return vcs_write_text("/.vcs/NEXT", buf);
}

// Reads current HEAD commit id into buffer.
static int get_head(char *out, int n)
{
    return vcs_read_text("/.vcs/HEAD", out, n);
}

// Updates HEAD to given commit id.
static int set_head(const char *id)
{
    return vcs_write_text("/.vcs/HEAD", id);
}

// Writes commit metadata file.
// Includes id, parent, and message.
static int write_meta(const char *id, const char *parent, const char *msg)
{
    char path[VCS_MAX_PATH];
    char buf[VCS_MAX_TEXT];

    // Build metadata file path.
    snprintf(path, sizeof(path), "/.vcs/commits/%s/meta", id);

    // Format metadata contents.
    snprintf(buf, sizeof(buf),
        "id=%s\n"
        "parent=%s\n"
        "message=%s\n",
        id,
        parent ? parent : "",
        msg ? msg : "");

    // Write metadata file.
    return vcs_write_text(path, buf);
}

// Creates a new commit snapshot of the current filesystem.
int vcs_commit(const char *message)
{
    // Ensure VCS is initialized.
    if (fs_lookup_path(&fs, "/.vcs") < 0)
    {
        fprintf(stderr, "VCS not initialized. Run: init\n");
        return -1;
    }

    // Get next commit id.
    int next = get_next_id();
    if (next < 0)
    {
        return -1;
    }

    char id[32];
    char parent[64];
    char commit_dir[VCS_MAX_PATH];
    char tree_dir[VCS_MAX_PATH];

    // Generate commit id string.
    vcs_make_commit_id(next, id, sizeof(id));

    // Build commit paths.
    snprintf(commit_dir, sizeof(commit_dir), "/.vcs/commits/%s", id);
    snprintf(tree_dir, sizeof(tree_dir), "/.vcs/commits/%s/tree", id);

    // Create commit directories.
    if (vcs_ensure_exists(commit_dir, DIR_TYPE) < 0)
    {
        return -1;
    }

    if (vcs_ensure_exists(tree_dir, DIR_TYPE) < 0)
    {
        return -1;
    }

    VcsEntry entries[VCS_MAX_ENTRIES];

    // List root directory entries.
    int count = list_dir(0, entries, VCS_MAX_ENTRIES);
    if (count < 0)
    {
        return -1;
    }

    // Copy all root entries except .vcs into commit tree.
    for (int i = 0; i < count; i++)
    {
        if (strcmp(entries[i].name, ".vcs") == 0)
        {
            continue;
        }

        char src[VCS_MAX_PATH];
        char dst[VCS_MAX_PATH];

        if (vcs_path_join(src, sizeof(src), "/", entries[i].name) != 0)
        {
            return -1;
        }

        if (vcs_path_join(dst, sizeof(dst), tree_dir, entries[i].name) != 0)
        {
            return -1;
        }

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

    // Read current HEAD as parent commit.
    if (get_head(parent, sizeof(parent)) != 0)
    {
        parent[0] = '\0';
    }

    // Write metadata and update state.
    if (write_meta(id, parent, message) != 0)
    {
        return -1;
    }

    if (set_head(id) != 0)
    {
        return -1;
    }

    if (set_next_id(next + 1) != 0)
    {
        return -1;
    }

    printf("committed %s\n", id);
    return 0;
}