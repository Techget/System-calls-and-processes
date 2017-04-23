#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>
#include <proc.h>

/*
 * Add your file-related functions here ...
 */

int 
sys_open(const char *filename, int flags, mode_t mode, int *retval){
	kprintf("open(%s, %d, %d)\n", filename, flags, mode);
	*retval = 0;
	return 0;
}

int 
sys_dup2(int oldfd, int newfd, int *retval){
	kprintf("dup2(%d, %d)\n", oldfd, newfd);
	*retval = 0;
	return 0;
}

int 
sys_close(int fd, int *retval){
	kprintf("close(%d )\n", fd);
	*retval = 0;
	return 0;
}

int 
sys_read(int fd, void *buf, size_t count, int *retval){
	int result=0;
	// verify the validaty of file handler
	if(fd >= OPEN_MAX || fd < 0) {
		return EBADF;
	}

	if(curproc->p_fdtable->fdt[fd] == NULL) {
		return EBADF;
	}

	if(curproc->p_fdtable->fdt[fd]->open_file == NULL) {
		return EBADF;
	}
	// open mode goes wrong, can't read with write only access
	if(curproc->p_fdtable->fdt[fd]->flags == O_WRONLY){
		return EBADF;
	}
	
	struct iovec * io_vector;
	io_vector = (struct iovec *)kmalloc(sizeof(struct iovec));
	struct uio * uio_temp;
	uio_temp = (struct uio *)kmalloc(sizeof(struct uio));
	void *k_buf;
	// sizeof(*buf)*count, *buf may be char * or int *
	// so sizeof(char)*count is the size of k_buf we need.
	k_buf = kmalloc(sizeof(*buf)*count);
	// system can't allocate such large memory as required, 
	// then return EINVAL indicate that it is invalid or not suitable.
	if(k_buf == NULL) {
		kfree(io_vector);
		kfree(uio_temp);
		return EINVAL;
	}

	// Initialize an iovec and uio for kernel I/O.
	uio_kinit(io_vector, uio_temp, k_buf, count, 
		curproc->p_fdtable->fdt[fd]->offset, UIO_READ);
	// use UIO_USERSPACE instead of UIO_SYSSPACE
	// uio_temp->uio_segflg = UIO_USERSPACE; // confused here

	result = VOP_READ(curproc->p_fdtable->fdt[fd]->open_file->vn, uio_temp);

	if(result) {
		kfree(io_vector);
		kfree(uio_temp);
		kfree(k_buf);
		return result;
	}

	// copy the buffer in kernel to userspace buf
	result = copyout((const void*)k_buf, (userptr_t)buf, count);

	// update the offset field in file descriptor
	curproc->p_fdtable->fdt[fd]->offset = uio_temp->uio_offset;

	// return value is the byte count it reads.
	*retval = count - uio_temp->uio_resid;
	// retval should be 0 if it signifys end of file

	kfree(io_vector);
	kfree(uio_temp);
	kfree(k_buf);

	return 0;
}

int 
sys_write(int fd, const void *buf, size_t count, int *retval){
	kprintf("write(%d, -, %d)\n", fd, count);
	*retval = 0;
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf));

	kfree(kbuf);
	return 0;

}

off_t 
sys_lseek(int fd, off_t offset, int whence, int *retval, int *retval1){
	kprintf("lseek(%d, - , %d)\n", fd, whence);
	*retval = 0;
	*retval1 = 0;
	void *kbuf;
	kbuf = kmalloc(sizeof(offset));

	kfree(kbuf);
	return 0;
}

void fd_table_init(void){
	int i;

	curproc->p_fdtable = (struct fd_table *)kmalloc(sizeof(struct fd_table));

	// set all the field in curproc->p_fdtable to null
	for(i=0;i<OPEN_MAX;i++){
		curproc->p_fdtable->fdt[i] = NULL;
	}

	// stdin
	curproc->p_fdtable->fdt[0] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[0]->open_file = open_file_table[0];
	curproc->p_fdtable->fdt[0]->flags = O_RDONLY;
	// stdout
	curproc->p_fdtable->fdt[1] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[1]->open_file = open_file_table[1];
	curproc->p_fdtable->fdt[1]->flags = O_WRONLY;
	// stderr
	curproc->p_fdtable->fdt[2] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[2]->open_file = open_file_table[2];
	curproc->p_fdtable->fdt[2]->flags = O_WRONLY;
}

void opf_table_init(){
	// set all the field of open_file_table to null in the beginning
	int i = 0;
	for(i = 0; i<OPF_TABLE_SIZE; i++){
		open_file_table[i] = NULL;
	}

	// create struct opf for stdin/out/err
	struct opf * opf_stdin = (struct opf *)kmalloc(sizeof(struct opf));
	struct opf * opf_stdout = (struct opf *)kmalloc(sizeof(struct opf));
	struct opf * opf_stderr = (struct opf *)kmalloc(sizeof(struct opf));

	// set refcount to 1, since at least the system will need to use
	// stdin/out/err
	opf_stdin->refcount = 1;
	opf_stdout->refcount = 1;
	opf_stderr->refcount = 1;

	// vnode for stdin/out/err
	struct vnode * vn_stdin;
	struct vnode * vn_stdout;
	struct vnode * vn_stderr;

	char c1[] = "con:";
	char c2[] = "con:";
	char c3[] = "con:";

	// open the correponding vnode, if not success clean up
	// the memory allocated.
	if(vfs_open(c1, O_RDONLY, 0, &vn_stdin)){
		kfree(opf_stdin);
		kfree(opf_stdout);
		kfree(opf_stderr);
		vfs_close(vn_stdin);
		kprintf("something wrong\n");
	}
	if(vfs_open(c2, O_WRONLY, 0, &vn_stdout)){
		kfree(opf_stdin);
		kfree(opf_stdout);
		kfree(opf_stderr);
		vfs_close(vn_stdin);
		vfs_close(vn_stdout);
		kprintf("something wrong\n");
	}
	if(vfs_open(c3, O_WRONLY, 0, &vn_stderr)){
		kfree(opf_stdin);
		kfree(opf_stdout);
		kfree(opf_stderr);
		vfs_close(vn_stdin);
		vfs_close(vn_stdout);
		vfs_close(vn_stderr);
		kprintf("something wrong\n");
	}

	// initialize offset to 0
	opf_stdin->offset = 0;
	opf_stdout->offset = 0;
	opf_stderr->offset = 0;

	// set correponding field of struct opf
	opf_stdin->vn = vn_stdin;
	opf_stdout->vn = vn_stdout;
	opf_stderr->vn = vn_stderr;

	// set open_file_table[0,1,2] to stdin/out/err
	open_file_table[0] = opf_stdin;
	open_file_table[1] = opf_stdout;
	open_file_table[2] = opf_stderr;
}