#include "../vfs/vfs.h"
#include "../../lib/logging.h"
#include "../../lib/refcount.h"
#include "../../mem/alloc.h"
#include "../../mem/paging.h"
#include "../../mem/utils.h"
#include "../../mem/vmm.h"
#include "../../drivers/vga/vgahandler.h"
#include "../../lib/str.h"
#include "../../task/sync/spinlock.h"
#include <stddef.h>
#include <stdint.h>

struct procfs {
    uint32_t procfs_count;
    struct vfs_op_tbl procfs_ops;
    struct vfs_fs* procfs_fs;
    spinlock_t procfs_lock;
    struct vfs_directory_entry* procfs_dir_entries;
};

static struct procfs g_procfs;

static struct vfs_directory_list* procfs_build_dirlist() {
    struct vfs_directory_list* list = kmalloc(sizeof(struct vfs_directory_list));
    if (!list)
        return NULL;

    list->head = g_procfs.procfs_dir_entries;
    list->tail = NULL;

    struct vfs_directory_entry* iter = list->head;
    while (iter) {
        list->tail = iter;
        iter = iter->next;
    }

    return list;
}

void procfs_add_entry(const char* name, int type) {
    struct vfs_directory_entry* entry = kmalloc(sizeof(struct vfs_directory_entry));
    if (!entry)
        return;

    flopstrcopy(entry->name, name, VFS_MAX_FILE_NAME);
    entry->type = type;
    entry->next = NULL;

    spinlock(&g_procfs.procfs_lock);
    if (!g_procfs.procfs_dir_entries) {
        g_procfs.procfs_dir_entries = entry;
    } else {
        struct vfs_directory_entry* iter = g_procfs.procfs_dir_entries;
        while (iter->next)
            iter = iter->next;
        iter->next = entry;
    }
    g_procfs.procfs_count++;
    spinlock_unlock(&g_procfs.procfs_lock, true);
}

static struct vfs_node* procfs_open(struct vfs_node* node, char* path) {
    return node;
}

static int procfs_close(struct vfs_node* node) {
    return 0;
}

static int procfs_read(struct vfs_node* node, unsigned char* buf, unsigned long size) {
    if (!node || !buf || size == 0)
        return 0;

    size_t len = flopstrlen(node->name);
    if (len > size)
        len = size;
    flopstrcopy((char*) buf, node->name, len);
    return (int) len;
}

static int procfs_write(struct vfs_node* node, unsigned char* buf, unsigned long size) {
    return -1;
}

static void* procfs_mount(char* dev, char* path, int flags) {
    log("procfs: mount called\n", 0x0F);

    if (!g_procfs.procfs_fs) {
        g_procfs.procfs_fs = kmalloc(sizeof(struct vfs_fs));
        if (!g_procfs.procfs_fs)
            return NULL;

        g_procfs.procfs_fs->filesystem_type = VFS_TYPE_PROCFS;
        g_procfs.procfs_fs->op_table = g_procfs.procfs_ops;
        g_procfs.procfs_fs->previous = NULL;
    }

    return g_procfs.procfs_fs;
}

static int procfs_unmount(struct vfs_mountpoint* mp, char* path) {
    return 0;
}

static int procfs_create(struct vfs_mountpoint* mp, char* name) {
    return -1;
}

static int procfs_delete(struct vfs_mountpoint* mp, char* name) {
    return -1;
}

static int procfs_unlink(struct vfs_mountpoint* mp, char* name) {
    return -1;
}

static int procfs_mkdir(struct vfs_mountpoint* mp, char* name, uint32_t flags) {
    return -1;
}

static int procfs_rmdir(struct vfs_mountpoint* mp, char* name) {
    return -1;
}

static int procfs_rename(struct vfs_mountpoint* mp, char* oldname, char* newname) {
    return -1;
}

static int procfs_ctrl(struct vfs_node* node, unsigned long cmd, unsigned long arg) {
    return -1;
}

static int procfs_seek(struct vfs_node* node, unsigned long offset, unsigned char whence) {
    return -1;
}

static struct vfs_directory_list* procfs_listdir(struct vfs_mountpoint* mp, char* path) {
    return procfs_build_dirlist();
}

static int procfs_stat(const char* path, struct stat* st) {
    if (!st)
        return -1;
    flop_memset(st, 0, sizeof(struct stat));
    st->st_mode = 0x4000;
    st->st_size = 0;
    return 0;
}

static int procfs_fstat(struct vfs_node* node, struct stat* st) {
    return procfs_stat(node ? node->name : NULL, st);
}

static int procfs_lstat(const char* path, struct stat* st) {
    return procfs_stat(path, st);
}

static int procfs_truncate(struct vfs_node* node, uint64_t length) {
    return -1;
}

static int procfs_ioctl(struct vfs_node* node, unsigned long cmd, unsigned long arg) {
    return -1;
}

static int procfs_link(struct vfs_mountpoint* mp, char* oldname, char* newname) {
    return -1;
}

void procfs_init() {
    spinlock_init(&g_procfs.procfs_lock);
    g_procfs.procfs_count = 0;
    g_procfs.procfs_dir_entries = NULL;

    procfs_add_entry("cpuinfo", VFS_FILE);
    procfs_add_entry("meminfo", VFS_FILE);

    g_procfs.procfs_ops.open = procfs_open;
    g_procfs.procfs_ops.close = procfs_close;
    g_procfs.procfs_ops.read = procfs_read;
    g_procfs.procfs_ops.write = procfs_write;
    g_procfs.procfs_ops.mount = procfs_mount;
    g_procfs.procfs_ops.unmount = procfs_unmount;
    g_procfs.procfs_ops.create = procfs_create;
    g_procfs.procfs_ops.delete = procfs_delete;
    g_procfs.procfs_ops.unlink = procfs_unlink;
    g_procfs.procfs_ops.mkdir = procfs_mkdir;
    g_procfs.procfs_ops.rmdir = procfs_rmdir;
    g_procfs.procfs_ops.rename = procfs_rename;
    g_procfs.procfs_ops.ctrl = procfs_ctrl;
    g_procfs.procfs_ops.seek = procfs_seek;
    g_procfs.procfs_ops.listdir = procfs_listdir;
    g_procfs.procfs_ops.stat = procfs_stat;
    g_procfs.procfs_ops.fstat = procfs_fstat;
    g_procfs.procfs_ops.lstat = procfs_lstat;
    g_procfs.procfs_ops.truncate = procfs_truncate;
    g_procfs.procfs_ops.ioctl = procfs_ioctl;
    g_procfs.procfs_ops.link = procfs_link;

    log("procfs: init - ok\n", GREEN);
}
