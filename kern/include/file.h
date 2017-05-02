/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

// Self-defined global open file table size
#define OPF_TABLE_SIZE 128

// Data structure for file descriptor
struct fd {
    struct opf * open_file; 
    int flags;
};

// Data structure for file descriptor table
struct fd_table {
	struct fd * fdt[OPEN_MAX];
};

// Data structure for open file table entry
struct opf{
    struct vnode *vn;
    int refcount;
    off_t offset;
};

// This array is the global open file table
extern struct opf * open_file_table[OPF_TABLE_SIZE];

// Declarations of system calls
int sys_open(const char *filename, int flags, mode_t mode, int *retval);
int sys_dup2(int oldfd, int newfd, int *retval);
int sys_close(int fd, int *retval);
int sys_read(int fd, void *buf, size_t count, int *retval);
int sys_write(int fd, const void *buf, size_t count, int *retval);
off_t sys_lseek(int fd, off_t offset, int whence, off_t * retval64);

// Initialization function
void opf_table_init(void);
void fd_table_init(void);

#endif /* _FILE_H_ */
