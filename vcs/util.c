#include <stdio.h>
#include <string.h>
#include "internal.h"

int vcs_path_join(char *out, size_t out_sz, const char *a, const char *b)
{
	if (!out || !a || !b) return -1;

	if (strcmp(a, "/") == 0)
    {
		if (snprintf(out, out_sz, "/%s", b) >= (int)out_sz) return -1;
	} else {
		if (snprintf(out, out_sz, "%s/%s", a, b) >= (int)out_sz) return -1;
	}

	return 0;
}

void vcs_make_commit_id(int n, char *out, size_t out_sz)
{
	snprintf(out, out_sz, "c%06d", n);
}