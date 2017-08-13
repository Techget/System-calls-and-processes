# System calls and processes assignments designs

Following is the design for this project, and files/functions/structures related or may be affected. 

(details specification refers to: http://cgi.cse.unsw.edu.au/~cs3231/17s1/assignments/asst2/)

/*********************Work flow and some considerations ****************/

Sys_call
(user) make syscall in instruction, eg. open(fname, flag, mode)
(user) set register, trapframe push in stack, and call syscall(tf) in trap.c
Context switch 
(kernel) mips_trap, call syscall function defined in syscall.c
(kernel) make syscall for file operation according to the `callno` passed in, return value store in trap_frame register
(user) return to user program

File descriptor table & open file table initialization
    Initialize file descriptor tables in `runprogram()`.
    Initialize file descriptor 0,1,2
    Open file table initialize in `kmain()`

Open/Close make use of vfs_open, vfs_open, which are vfs operations, for operation like read/write etc. make use of vop of vnode, vop_read/vop_write.

Syscall is executed in kernel mode, where interrupt is disabled, so 

With every open() system call, user gets its own private reference number known as file descriptor. 2. Separate entries are created in user file descriptor and global file table, but only the reference count is increased in the vnode table. 3. all the system calls related to file handling use these tables for data manipulation.

Global File table. It contains information that is global to the kernel e.g. the byte offset in the file where the user's next read/write will start and the access rights allowed to the opening process. 

Process File Descriptor table. It is local to every process and contains information like the identifiers of the files opened by the process. Whenever, a process creates a file, it gets an index from this table primarily known as File Descriptor.

We need global open file table for dup2() and fork()

dup() system call doesn’t create a separate entry in the global file table like the open() system call, instead it just increments the count field of the entry pointed to by the given input file descriptor in the global file table.

/********************** syscall.c ***************************/

In switch:
    Add 6 new cases for sys_open, sys_close, sys_read, sys_write, sys_dup2, sys_lseek
    Call functions in file.c

/*********************** proc.h *****************************/

Add following structures:

struct proc{
    char *p_name;            /* Name of this process */
    struct spinlock p_lock;        /* Lock for this structure */
    unsigned p_numthreads;        /* Number of threads in this process */
    /* VM */
    struct addrspace *p_addrspace;    /* virtual address space */
    /* VFS */
    struct vnode *p_cwd;        /* current working directory */
   /* File descriptor table */
    struct fd_table * fdesc_table;    
}

/************************** file.h *****************************/

Add following structures:

struct fd {
    struct opf * open_file;
    int flags;
};

struct fd_table {
    struct fd * fdt[OPEN_MAX];
};

struct opf{
    struct vnode *vn;
    int refcount;
    off_t offset;
};

extern struct opf * open_file_table[OPF_TABLE_SIZE];



/***********************syscall function design ****************************/
In file.c
/*################### sys_open ########################*/
sys_open:
1. file descriptor table:
    fd start at 3
    check if fd taken, then increase fd
    check MAX

2. vfs_open:
    filepath from userlevel to kernel level
    kmalloc buffer
    vfs_open(buffer, flags, mode)
    error handle
    free buffer in every cases

3. Safety check:
    input boundary check
    memory free

4. Each open() returns a file descriptor to the process, and the corresponding entry in the user file descriptor table points to a unique entry in the global file table even though a file(/var/file1) is opened more than once.

/*################### sys_read ########################*/
sys_read:
1. input check:
    fd boundary check
    fd emptiness check

2. VOP_READ:
    init struct iovec, ->ubase = buf
    init struct uio, ->uio_iov = &iov
    VOP_READ()
    error handle
    offset reset

3. safety check:
    input boundary check
    memory free

/*################### sys_write ########################*/
sys_write:
1. input check:
    fd boundary check
    fd emptiness check

2. VOP_WRITE:
    init struct iovec, ->ubase = buf
    init struct uio, ->uio_iov = &iov
    VOP_WRITE()
    error handle
    offset reset

3. Safety check:
    fdtable lock
    input boundary check
    memory free

/*################### sys_close ########################*/
sys_close:
1. input check:
    fd boundary check
    fd emptiness check

2. VOP_CLOSE:
    free fdtable
    fdtable refcount

/*################### sys_dup2 ########################*/
sys_dup2:
1. input check:
    fd boundary check
    fd emptiness check
    fdold != fdnew check
    
2. file descriptor copy
    all old fd parameters copy to new fd

3. Safety check:
    input boundary check
    memory free

/*################### sys_lseek ########################*/
sys_lseek:
1. input check:
    fd boundary check
    fd emptiness check

2. Whence options:
    switch:{SEEK_SET, SEEK_CUR, SEEK_END}
    VOP_TRYSEEK
    error handle

3. Safety check:
    input boundary check
memory free
