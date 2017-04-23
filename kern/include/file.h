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
};

struct fd_table {
	struct fd * fdt[OPEN_MAX];
};

struct opf{
    struct vnode *vn;
    int refcount;
};

extern struct opf * open_file_table[OPEN_MAX];
/*
 * Put your function declarations and data types here ...
 */

int sys_open(const char *filename, int flags, mode_t mode, int *retval);
int sys_dup2(int oldfd, int newfd, int *retval);
int sys_close(int fd, int *retval);
int sys_read(int fd, void *buf, size_t count, int *retval);
int sys_write(int fd, const void *buf, size_t count, int *retval);
off_t sys_lseek(int fd, off_t offset, int whence, int *retval,int *retval1);

void opf_table_init(void);
void fd_table_init(void);

#endif /* _FILE_H_ */
