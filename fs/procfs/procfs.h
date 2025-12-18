#ifndef PROCFS_H
#define PROCFS_H

#include "../vfs/vfs.h"
#include <stdint.h>
#include <stddef.h>

struct procfs;

void procfs_init(void);

void procfs_add_entry(const char* name, int type);

struct vfs_directory_list* procfs_build_dirlist(void);

size_t procfs_read(struct vfs_node* node, void* buf, size_t count);

int procfs_stat(const char* path, struct stat* st);
int procfs_fstat(struct vfs_node* node, struct stat* st);
int procfs_lstat(const char* path, struct stat* st);

#endif
