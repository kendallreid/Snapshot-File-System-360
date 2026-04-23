#include <stdio.h>
#include "vcs.h"
#include "helpers.h"

int vcs_init(void)
{
	if (vcs_ensure_exists("/.vcs", DIR_TYPE) < 0) return -1;
	if (vcs_ensure_exists("/.vcs/commits", DIR_TYPE) < 0) return -1;
	if (vcs_write_text("/.vcs/HEAD", "") != 0) return -1;
	if (vcs_write_text("/.vcs/NEXT", "1") != 0) return -1;

	printf("initialized VCS\n");
	return 0;
}