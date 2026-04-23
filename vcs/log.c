#include <stdio.h>
#include <string.h>
#include "vcs.h"
#include "helpers.h"

#define VCS_MAX_PATH 256
#define VCS_MAX_TEXT 1024

// Extracts the value of a key from a metadata text block.
// The Expected format is "key=value" on separate lines.
// Writes the result into out and returns 0 on success, or -1 if not found.
static int meta_field(const char *meta, const char *key, char *out, int n)
{
    // Length of the key we are searching for.
    size_t k = strlen(key);
    const char *p = meta;

    // Scan line by line through the metadata text.
    while (*p)
    {
        // Check whether the current line starts with "key=".
        if (strncmp(p, key, k) == 0 && p[k] == '=')
        {
            p += k + 1;

            int i = 0;

            // Copy characters until newline or buffer is full.
            while (*p && *p != '\n' && i < n - 1)
            {
                out[i++] = *p++;
            }

            out[i] = '\0';
            return 0;
        }

        // Skip to the next line.
        while (*p && *p != '\n')
        {
            p++;
        }

        if (*p == '\n')
        {
            p++;
        }
    }

    // If the key was not found, return an empty string when possible.
    if (n > 0)
    {
        out[0] = '\0';
    }

    return -1;
}

// Prints the commit history starting from HEAD and walking through parent links.
int vcs_log(void)
{
    char cur[64];

    // Read the current HEAD commit id.
    if (vcs_read_text("/.vcs/HEAD", cur, sizeof(cur)) != 0 || cur[0] == '\0')
    {
        printf("no commits\n");
        return 0;
    }

    // Walk backward through the commit chain.
    while (cur[0])
    {
        char path[VCS_MAX_PATH];
        char meta[VCS_MAX_TEXT];
        char parent[64];
        char msg[256];

        // Build the path to the current commit metadata file.
        snprintf(path, sizeof(path), "/.vcs/commits/%s/meta", cur);

        // Read metadata for the current commit.
        if (vcs_read_text(path, meta, sizeof(meta)) != 0)
        {
            return -1;
        }

        // Extract parent id and commit message from metadata.
        meta_field(meta, "parent", parent, sizeof(parent));
        meta_field(meta, "message", msg, sizeof(msg));

        // Print the current commit entry.
        printf("commit %s\n", cur);
        printf("message: %s\n\n", msg);

        // Stop when there is no parent commit.
        if (parent[0] == '\0')
        {
            break;
        }

        // Move to the parent commit.
        strncpy(cur, parent, sizeof(cur) - 1);
        cur[sizeof(cur) - 1] = '\0';
    }

    return 0;
}