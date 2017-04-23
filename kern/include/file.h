/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

struct fd {
    struct opf * open_file; // the index of opfile_table
    off_t offset;
}

struct fd_table {
	struct fd * fdt[MAX_FILES];
}

enum opf_status{ READ, WRITE}

struct opf{
    enum opf_status *rw;
    struct vnode *vn;
    int refcount;
}

extern struct opf * open_file_table[MAX_FILES];
/*
 * Put your function declarations and data types here ...
 */

// int sys_open(const char *filename, int flags, mode_t mode);
// int sys_dup2(int oldfd, int newfd);
// int sys_close(int fd);
// int sys_read(int fd, void *buf, size_t count);
// int sys_write(int fd, const void *buf, size_t count);
// off_t sys_lseek(int fd, off_t offset, int whence);

void opf_table_init();
void fd_table_init();

#endif /* _FILE_H_ */
