#ifndef VCS_H
#define VCS_H

int vcs_init(void);
int vcs_commit(const char *message);
int vcs_log(void);
int vcs_checkout(const char *commit_id);

#endif