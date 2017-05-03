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
#include <synch.h>

/*
*	deal with open system call
*/
int sys_open(const char *filename, int flags, mode_t mode, int *retval){
	int result = 0;
	int index = 0;
	size_t offset = 0;
	int i=0;
	size_t len;
	// check flags
	if(flags<0 ||flags>127) {
		return EINVAL;
	}

	struct vnode *vn;
	struct opf *ofile;

	char *kbuf;
	kbuf = (char *)kmalloc(sizeof(char)*PATH_MAX);

	// copy file name string to kernel buffer
	result = copyinstr((const_userptr_t)filename, kbuf, PATH_MAX, &len);
	if(result) {
		kfree(kbuf);
		return result;
	}

	// get the lock of per process fd table
	lock_acquire(curproc->p_fdtable->lk);

	// check file descriptor table
	while(curproc->p_fdtable->fdt[index]!=NULL){
		index++;
	}
	// Too many files opened
	if(index>=OPEN_MAX){
		kfree(kbuf);
		lock_release(curproc->p_fdtable->lk);
		return ENFILE;
	}

	// check open file table, get the lock first
	lock_acquire(global_opf_table->lk);
	while(global_opf_table->open_file_table[i]!=NULL){
		i++;
	}
	// Too many files in open file table 
	if(i>=OPF_TABLE_SIZE){
		lock_release(global_opf_table->lk);
		lock_release(curproc->p_fdtable->lk);
		return ENFILE;
	}

	result = vfs_open(kbuf, flags, mode, &vn);
	// error on vfs_open
	if(result){
		kfree(kbuf);
		lock_release(global_opf_table->lk);
		lock_release(curproc->p_fdtable->lk);
		return result;
	}
	// create new file descriptor and put it into per process file descriptor table
	curproc->p_fdtable->fdt[index] = (struct fd *)kmalloc(sizeof(struct fd));

	// O_APPEND flag should set the offset the end of the file
	if(flags & O_APPEND){
		struct stat * file_stat = (struct stat *)kmalloc(sizeof(struct stat));
		result = VOP_STAT(vn, file_stat);
		if (result) {
			kfree(file_stat);
			return result;
		}

		offset = file_stat->st_size;

		kfree(file_stat);
	}else{
		offset = 0;
	}

	i=0;
	while(global_opf_table->open_file_table[i]!=NULL && global_opf_table->open_file_table[i]->vn != vn){
		i++;
	}

	// create new open file table entry
	if(global_opf_table->open_file_table[i]==NULL){
		ofile = (struct opf *)kmalloc(sizeof(struct opf));
		ofile->vn = vn;
		ofile->refcount = 1;
		ofile->offset = offset;
		ofile->lk = lock_create("open_file_table_entry");

		curproc->p_fdtable->fdt[index]->open_file = ofile;
		curproc->p_fdtable->fdt[index]->flags = flags;
		global_opf_table->open_file_table[i] = ofile;
	} else if(global_opf_table->open_file_table[i]->vn == vn){
		// link with existing open file table
		lock_acquire(global_opf_table->open_file_table[i]->lk);
		global_opf_table->open_file_table[i]->refcount++;
		curproc->p_fdtable->fdt[index]->open_file = global_opf_table->open_file_table[i];
		curproc->p_fdtable->fdt[index]->flags = flags;
		lock_release(global_opf_table->open_file_table[i]->lk);
	}
	// after add to global open file table release the lock
	lock_release(global_opf_table->lk);
	lock_release(curproc->p_fdtable->lk);

	// set return value as file descriptor
	*retval = index;
	kfree(kbuf);

	return 0;
}

/*
*	deal with close system call
*/
int sys_close(int fd, int *retval){
	//bad file descriptor
	if(fd >= OPEN_MAX || fd < 0) {
		return EBADF;
	}
	//check file descriptor
	if(curproc->p_fdtable->fdt[fd] == NULL) {
		return EBADF;
	}
	//check open file
	if(curproc->p_fdtable->fdt[fd]->open_file == NULL) {
		return EBADF;
	}

	//check vnode
	if(curproc->p_fdtable->fdt[fd]->open_file->vn == NULL) {
		return EBADF;
	}

	// get the lock for both table and the corresponding opf_table_entry
	lock_acquire(curproc->p_fdtable->lk);
	lock_acquire(global_opf_table->lk);
	lock_acquire(curproc->p_fdtable->fdt[fd]->open_file->lk);

	//close vnode
	// kprintf("vn_refcount: %d\n", curproc->p_fdtable->fdt[fd]->open_file->vn->vn_refcount);
	if(curproc->p_fdtable->fdt[fd]->open_file->vn->vn_refcount == 1) {
		// kprintf("vn_refcount: %d\n", curproc->p_fdtable->fdt[fd]->open_file->vn->vn_refcount);
		vfs_close(curproc->p_fdtable->fdt[fd]->open_file->vn);
		curproc->p_fdtable->fdt[fd]->open_file->vn = NULL;
	} else {
		curproc->p_fdtable->fdt[fd]->open_file->vn->vn_refcount -= 1;
	}

	// close open file
	// kprintf("open_file->refcount: %d\n", curproc->p_fdtable->fdt[fd]->open_file->refcount);
	if(curproc->p_fdtable->fdt[fd]->open_file->refcount == 1){
		// curproc->p_fdtable->fdt[fd]->open_file->vn = NULL;
		int i = 0;
		for(i=0; i<OPF_TABLE_SIZE; i++){
			if(curproc->p_fdtable->fdt[fd]->open_file == global_opf_table->open_file_table[i]){
				break;
			}
		}
		// release and destroy the lock since this file descriptor is going to be
		// destroyed
		// kprintf("lock_destroy!!!!! \n");
		lock_release(curproc->p_fdtable->fdt[fd]->open_file->lk);
		lock_destroy(curproc->p_fdtable->fdt[fd]->open_file->lk);
		kfree(curproc->p_fdtable->fdt[fd]->open_file);
		curproc->p_fdtable->fdt[fd]->open_file = NULL;
		global_opf_table->open_file_table[i] = NULL;
	} else {
		curproc->p_fdtable->fdt[fd]->open_file->refcount -= 1;
	}

	// If hold the lock for open file entry, should release it
	if (curproc->p_fdtable->fdt[fd]->open_file != NULL 
		&& lock_do_i_hold(curproc->p_fdtable->fdt[fd]->open_file->lk)) {
		lock_release(curproc->p_fdtable->fdt[fd]->open_file->lk);
	}
	
	// close fd, free file descriptor structure
	kfree(curproc->p_fdtable->fdt[fd]);
	curproc->p_fdtable->fdt[fd] = NULL;

	lock_release(global_opf_table->lk);
	lock_release(curproc->p_fdtable->lk);

	*retval = 0;

	return 0;
}

/*
*	deal with dup2 system call
*/
int sys_dup2(int oldfd, int newfd, int *retval){
	int result = 0;

	// bad file descriptor number
	if (oldfd >= OPEN_MAX || oldfd < 0 || newfd >= OPEN_MAX || newfd < 0) {
		return EBADF;
	}

	// Same file descriptor number
	if (oldfd == newfd) {
		*retval = newfd;
		return EINVAL;
	}

	// check oldfd
	if (curproc->p_fdtable->fdt[oldfd] == NULL) {
		return EBADF;
	}
	// check newfd
	if (curproc->p_fdtable->fdt[newfd] != NULL) {
		result = sys_close(newfd, retval);
		if(result) {
			return EBADF;
		}
	}
	// we need to corresponding openfile entry lock
	lock_acquire(curproc->p_fdtable->lk);

	// create a new file descriptor instance and assign it to newfd
	curproc->p_fdtable->fdt[newfd] = (struct fd*)kmalloc(sizeof(struct fd));

	// copy the old fd to new fd
	curproc->p_fdtable->fdt[newfd]->open_file = curproc->p_fdtable->fdt[oldfd]->open_file;
	curproc->p_fdtable->fdt[newfd]->flags = curproc->p_fdtable->fdt[oldfd]->flags;

	// increment the refcount in open file table corresponding entry & vnode
	curproc->p_fdtable->fdt[newfd]->open_file->refcount++;
	// add the vn_refcount manually.
	curproc->p_fdtable->fdt[newfd]->open_file->vn->vn_refcount++;

	lock_release(curproc->p_fdtable->lk);

	*retval = newfd;

	return 0;
}

/*
*	deal with read system call
*/
int sys_read(int fd, void *buf, size_t count, int *retval){
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
	// iov_vector will be needed for uio
	struct iovec * io_vector;
	io_vector = (struct iovec *)kmalloc(sizeof(struct iovec));
	struct uio * uio_temp;
	uio_temp = (struct uio *)kmalloc(sizeof(struct uio));
	// initialize uio, copy directly from kernel to user mode buffer
	// so we need to set the address space to user mode address space
	io_vector->iov_ubase = (userptr_t)buf;
	io_vector->iov_len = count;
	uio_temp->uio_iov = io_vector;
	uio_temp->uio_iovcnt = 1;
	uio_temp->uio_offset = curproc->p_fdtable->fdt[fd]->open_file->offset;
	uio_temp->uio_resid = count;
	uio_temp->uio_segflg = UIO_USERSPACE;
	uio_temp->uio_rw = UIO_READ;
	uio_temp->uio_space = curproc->p_addrspace;

	lock_acquire(curproc->p_fdtable->fdt[fd]->open_file->lk);

	result = VOP_READ(curproc->p_fdtable->fdt[fd]->open_file->vn, uio_temp);

	if (result) {
		kfree(io_vector);
		kfree(uio_temp);
		return result;
	}
	curproc->p_fdtable->fdt[fd]->open_file->offset = uio_temp->uio_offset;

	lock_release(curproc->p_fdtable->fdt[fd]->open_file->lk);

	*retval = count - uio_temp->uio_resid;

	kfree(io_vector);
	kfree(uio_temp);

	return 0;
}

/*
*	deal with write system call
*/
int sys_write(int fd, const void *buf, size_t count, int *retval){
	int result = 0;

	// bad file descriptor
	if(fd >= OPEN_MAX || fd < 0) {
		kprintf("W EBADF fd\n");
		return EBADF;
	}
	// check file
	if(curproc->p_fdtable->fdt[fd] == NULL) {
		kprintf("W EBADF fdt\n");
		return EBADF;
	}

	// check read only file
	if(curproc->p_fdtable->fdt[fd]->flags == O_RDONLY) {
		kprintf("W EBADF ro\n");
		return EBADF;
	}

	struct iovec iov;
	struct uio ku;

	// create kernel buffer
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf)*count);
	if(kbuf == NULL) {
		return EINVAL;
	}
	// copy user buffer to kernel buffer
	result = copyin((const_userptr_t)buf, kbuf, count);
	if (result) {
		kfree(kbuf);
		return result;
	}

	uio_kinit(&iov, &ku, kbuf, count ,
		curproc->p_fdtable->fdt[fd]->open_file->offset, UIO_WRITE);

	lock_acquire(curproc->p_fdtable->fdt[fd]->open_file->lk);
	// make use of vnode operation
	result = VOP_WRITE(curproc->p_fdtable->fdt[fd]->open_file->vn, &ku);
	if (result) {
		kfree(kbuf);
		return result;
	}
	// update the offset
	curproc->p_fdtable->fdt[fd]->open_file->offset = ku.uio_offset;

	lock_release(curproc->p_fdtable->fdt[fd]->open_file->lk);

	*retval = count - ku.uio_resid;
	kfree(kbuf);

	return 0;
}

/*
*	deal with lseek system call
*/
off_t sys_lseek(int fd, off_t offset, int whence, off_t * retval64){
	int result;
	// sanity check
	// check fd validity
	if (fd >= OPEN_MAX || fd < 0){
		return EBADF;
	}

	if (curproc->p_fdtable->fdt[fd] == NULL || 
		curproc->p_fdtable->fdt[fd]->open_file == NULL){
		return EBADF;
	}
	// use isseekable to check the underlying device is seekable or not
	if (!VOP_ISSEEKABLE(curproc->p_fdtable->fdt[fd]->open_file->vn)) {
		return ESPIPE;
	}

	// get the file info 
	struct stat * file_stat = (struct stat *)kmalloc(sizeof(struct stat));
	result = VOP_STAT(curproc->p_fdtable->fdt[fd]->open_file->vn, file_stat);
	if (result) {
		kfree(file_stat);
		return result;
	}

	off_t final_pos, file_size;
	file_size = file_stat->st_size;

	// operate according the whence value, update the offset value
	// in open file table entry
	if (whence == SEEK_SET) {
		// use >= instead of >
		if (offset >= file_size) {
			kfree(file_stat);
			return EINVAL;
		}
		final_pos = offset;
	} else if (whence == SEEK_CUR) {
		off_t temp_offset = curproc->p_fdtable->fdt[fd]->open_file->offset + offset;
		if (temp_offset >= file_size) {
			kfree(file_stat);
			return EINVAL;
		}
		final_pos = temp_offset;
	} else if (whence == SEEK_END) {
		off_t temp_offset = file_size + offset;
		if (temp_offset <= 0) {
			kfree(file_stat);
			return EINVAL;
		}
		final_pos = temp_offset;
	} else {
		return EINVAL;
	}

	lock_acquire(curproc->p_fdtable->fdt[fd]->open_file->lk);
	// update the offset value in open file table entry
	curproc->p_fdtable->fdt[fd]->open_file->offset = final_pos;
	lock_release(curproc->p_fdtable->fdt[fd]->open_file->lk);

	*retval64 = final_pos;

	return 0;
}

/*
*	init function for per process file descriptor table
*/
void fd_table_init(void){
	int i;

	curproc->p_fdtable = (struct fd_table *)kmalloc(sizeof(struct fd_table));

	// initialize the lock
	curproc->p_fdtable->lk = lock_create("fdtable");

	// set all the field in curproc->p_fdtable to null
	for(i=0;i<OPEN_MAX;i++){
		curproc->p_fdtable->fdt[i] = NULL;
	}

	// stdin
	curproc->p_fdtable->fdt[0] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[0]->open_file = global_opf_table->open_file_table[0];
	curproc->p_fdtable->fdt[0]->flags = O_RDONLY;
	// stdout
	curproc->p_fdtable->fdt[1] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[1]->open_file = global_opf_table->open_file_table[1];
	curproc->p_fdtable->fdt[1]->flags = O_WRONLY;
	// stderr
	curproc->p_fdtable->fdt[2] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[2]->open_file = global_opf_table->open_file_table[2];
	curproc->p_fdtable->fdt[2]->flags = O_WRONLY;
}

/*
*	Init function for global open file table, allocate memory to global open file table
*	, initialize the stdin/out/err, and set other slots to NULL.
*/
void opf_table_init(){
	global_opf_table = (struct opf_table *)kmalloc(sizeof(struct opf_table));
	// create global open file table lock
	global_opf_table->lk = lock_create("global_open_file_table");

	// set all the field of open_file_table to null in the beginning
	int i = 0;
	for(i = 0; i<OPF_TABLE_SIZE; i++){
		global_opf_table->open_file_table[i] = NULL;
	}

	// create struct opf for stdin/out/err
	struct opf * opf_stdin = (struct opf *)kmalloc(sizeof(struct opf));
	struct opf * opf_stdout = (struct opf *)kmalloc(sizeof(struct opf));
	struct opf * opf_stderr = (struct opf *)kmalloc(sizeof(struct opf));

	// initialize the lock for each opf entry
	opf_stdin->lk = lock_create("opf_stdin");
	opf_stdout->lk = lock_create("opf_stdout");
	opf_stderr->lk = lock_create("opf_stderr");

	// set refcount to 1, since at least the system will need to use
	// stdin/out/err
	opf_stdin->refcount = 1;
	opf_stdout->refcount = 1;
	opf_stderr->refcount = 1;

	// vnode for stdin/out/err
	struct vnode * vn_stdin;
	struct vnode * vn_stdout;
	struct vnode * vn_stderr;

	// device name
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
	}
	if(vfs_open(c2, O_WRONLY, 0, &vn_stdout)){
		kfree(opf_stdin);
		kfree(opf_stdout);
		kfree(opf_stderr);
		vfs_close(vn_stdin);
		vfs_close(vn_stdout);
	}
	if(vfs_open(c3, O_WRONLY, 0, &vn_stderr)){
		kfree(opf_stdin);
		kfree(opf_stdout);
		kfree(opf_stderr);
		vfs_close(vn_stdin);
		vfs_close(vn_stdout);
		vfs_close(vn_stderr);
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
	global_opf_table->open_file_table[0] = opf_stdin;
	global_opf_table->open_file_table[1] = opf_stdout;
	global_opf_table->open_file_table[2] = opf_stderr;
}
