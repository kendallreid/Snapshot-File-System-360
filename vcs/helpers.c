#include <stdio.h>
#include <string.h>
#include "helpers.h"

// Joins two path components into a single path.
// Returns 0 on success, or -1 if the output buffer is invalid or too small.
int vcs_path_join(char *out, size_t n, const char *a, const char *b)
{
    // Validate input pointers.
    if (!out || !a || !b)
    {
        return -1;
    }

    // Handle root specially so the result is "/name" instead of "//name".
    if (strcmp(a, "/") == 0)
    {
        return snprintf(out, n, "/%s", b) < (int)n ? 0 : -1;
    }

    // Join normal paths as "a/b".
    return snprintf(out, n, "%s/%s", a, b) < (int)n ? 0 : -1;
}

// Ensures that a path exists in the filesystem.
// If it already exists, returns its inode.
// If not, creates it and then returns its inode.
int vcs_ensure_exists(const char *path, inode_type type)
{
    // Check whether the path already exists.
    int ino = fs_lookup_path(&fs, path);
    if (ino >= 0)
    {
        return ino;
    }

    // Create the path, then look it up again.
    fs_create(&fs, path, type);
    return fs_lookup_path(&fs, path);
}

// Writes a null terminated text string to a file.
// Returns the result of fs_write, or -1 on failure.
int vcs_write_text(const char *path, const char *text)
{
    // Ensure the target file exists.
    int ino = vcs_ensure_exists(path, FILE_TYPE);
    if (ino < 0)
    {
        return -1;
    }

    // Write the text contents into the file.
    return fs_write(&fs, ino, text, (int)strlen(text));
}

// Reads text from a file into the output buffer.
// Always null terminates the buffer when possible.
// Returns 0 on success, or -1 on failure.
int vcs_read_text(const char *path, char *out, int n)
{
    // Resolve the file path.
    int ino = fs_lookup_path(&fs, path);
    if (ino < 0)
    {
        if (n > 0)
        {
            out[0] = '\0';
        }

        return -1;
    }

    // Get file size.
    int size = fs_get_size(&fs, ino);
    if (size < 0)
    {
        if (n > 0)
        {
            out[0] = '\0';
        }

        return -1;
    }

    // Clamp read size so there is space for the null terminator.
    if (size >= n)
    {
        size = n - 1;
    }

    // Read file contents into the output buffer.
    if (fs_read(&fs, ino, out, size) < 0)
    {
        if (n > 0)
        {
            out[0] = '\0';
        }

        return -1;
    }

    // Null terminate the result string.
    out[size] = '\0';
    return 0;
}

// Builds a commit id string like "c000001" from an integer.
void vcs_make_commit_id(int n, char *out, size_t size)
{
    snprintf(out, size, "c%06d", n);
}