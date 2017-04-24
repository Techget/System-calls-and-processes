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
	int result = 0;
	int index = 3;
	int i=0;
	size_t len;

	if(!(flags==O_RDONLY || flags==O_WRONLY || flags==O_RDWR || flags==(O_RDWR|O_CREAT|O_TRUNC))) {
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
	// check file descriptor table
	while(curproc->p_fdtable->fdt[index]!=NULL){
		index++;
	}
	// Too many files opened
	if(index==OPEN_MAX){
		kfree(kbuf);
		return ENFILE;
	}

	// check open file table
	while(open_file_table[i]!=NULL){
		i++;
	}
	// Too many files in open file table 
	if(i==OPF_TABLE_SIZE){
		return ENFILE;
	}

	result = vfs_open(kbuf, flags, mode, &vn);
	// error on vfs_open
	if(result){
		kfree(kbuf);
		return result;
	}
	// create new file descriptor
	curproc->p_fdtable->fdt[index] = (struct fd *)kmalloc(sizeof(struct fd));

	i=0;
	while(open_file_table[i]!=NULL && open_file_table[i]->vn != vn){
		i++;
	}

	// create new open file table
	if(open_file_table[i]==NULL){
		ofile = (struct opf *)kmalloc(sizeof(struct opf));
		ofile->vn = vn;
		ofile->refcount = 1;
		ofile->offset = 0;

		curproc->p_fdtable->fdt[index]->open_file = ofile;
		curproc->p_fdtable->fdt[index]->flags = flags;
		open_file_table[i] = ofile;
	}
	// link with existing open file table
	if(open_file_table[i]->vn == vn){
		open_file_table[i]->refcount++;
		curproc->p_fdtable->fdt[index]->open_file = open_file_table[i];
		curproc->p_fdtable->fdt[index]->flags = flags;
	}

	// set return value as file descriptor
	*retval = index;
	kfree(kbuf);

	return 0;
}

int 
sys_dup2(int oldfd, int newfd, int *retval){
	int result = 0;

	// bad file descriptor number
	if (oldfd >= OPEN_MAX || oldfd < 0 || newfd >= OPEN_MAX || newfd < 0) {
		return EBADF;
	}

	// Same file descriptor number
	if (oldfd == newfd) {
		*retval = newfd;
		return 0;
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
	} else {
		curproc->p_fdtable->fdt[newfd] = (struct fd*)kmalloc(sizeof(struct fd));
	}

	// copy the old fd to new fd
	curproc->p_fdtable->fdt[newfd]->open_file = curproc->p_fdtable->fdt[oldfd]->open_file;
	curproc->p_fdtable->fdt[newfd]->flags = curproc->p_fdtable->fdt[oldfd]->flags;

	// increment the refcount in open file table corresponding entry & vnode
	curproc->p_fdtable->fdt[newfd]->open_file->refcount++;
	curproc->p_fdtable->fdt[newfd]->open_file->vn->vn_refcount++;

	*retval = newfd;

	return 0;
}

int 
sys_close(int fd, int *retval){
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

	//close vnode
	if(curproc->p_fdtable->fdt[fd]->open_file->vn->vn_refcount == 1) {
		vfs_close(curproc->p_fdtable->fdt[fd]->open_file->vn);
	} else {
		curproc->p_fdtable->fdt[fd]->open_file->vn->vn_refcount -= 1;
	}

	// close open file
	if(curproc->p_fdtable->fdt[fd]->open_file->refcount == 1){
		curproc->p_fdtable->fdt[fd]->open_file->vn = NULL;
		kfree(curproc->p_fdtable->fdt[fd]->open_file);
	}
 	else {
		curproc->p_fdtable->fdt[fd]->open_file->refcount -= 1;
	}

	// free file descriptor structure
	kfree(curproc->p_fdtable->fdt[fd]);
	curproc->p_fdtable->fdt[fd] = NULL;

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
	k_buf = kmalloc(sizeof(*buf) * count);
	// system can't allocate such large memory as required, 
	// then return EINVAL indicate that it is invalid or not suitable.
	if(k_buf == NULL) {
		kfree(io_vector);
		kfree(uio_temp);
		return EINVAL;
	}

	// Initialize an iovec and uio for kernel I/O.
	uio_kinit(io_vector, uio_temp, k_buf, count, 
		curproc->p_fdtable->fdt[fd]->open_file->offset, UIO_READ);
	// use UIO_USERSPACE instead of UIO_SYSSPACE, since 
	// uio_temp->uio_segflg = UIO_USERSPACE; 

	result = VOP_READ(curproc->p_fdtable->fdt[fd]->open_file->vn, uio_temp);

	if (result) {
		kfree(io_vector);
		kfree(uio_temp);
		kfree(k_buf);
		return result;
	}

	// copy the buffer in kernel to userspace buf
	result = copyout((const void*)k_buf, (userptr_t)buf, count);

	// update the offset field in file descriptor
	curproc->p_fdtable->fdt[fd]->open_file->offset = uio_temp->uio_offset;

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
	int result = 0;

	// bad file descriptor
	if(fd >= OPEN_MAX || fd < 0) {
		return EBADF;
	}
	// check file
	if(curproc->p_fdtable->fdt[fd] == NULL) {
		return EBADF;
	}

	// check read only file
	if(curproc->p_fdtable->fdt[fd]->flags == O_RDONLY) {
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

	result = VOP_WRITE(curproc->p_fdtable->fdt[fd]->open_file->vn, &ku);
	if (result) {
		kfree(kbuf);
		return result;
	}

	curproc->p_fdtable->fdt[fd]->open_file->offset = ku.uio_offset;

	*retval = count - ku.uio_resid;

	kfree(kbuf);

	return 0;
}

off_t 
sys_lseek(int fd, off_t offset, int whence, int *retval, int *retval1){
	int result;
	// ###### sanity check ########
	// check fd validity
	if (fd >= OPEN_MAX || fd < 0){
		return EBADF;
	}

	if (curproc->p_fdtable->fdt[fd] == NULL || 
		curproc->p_fdtable->fdt[fd]->open_file == NULL){
		return EBADF;
	}
	// use isseekable to check the underlying device is seekable or not
	if (VOP_ISSEEKABLE(curproc->p_fdtable->fdt[fd]->open_file->vn)) {
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
		off_t temp_offset = file_size - offset;
		if (temp_offset <= 0) {
			kfree(file_stat);
			return EINVAL;
		}
		final_pos = temp_offset;
	} else {
		return EINVAL;
	}

	// update the offset value in open file table entry
	curproc->p_fdtable->fdt[fd]->open_file->offset = final_pos;

	*retval = (uint32_t)((offset & 0xFFFFFFFF00000000) >> 32);
	*retval1 = (uint32_t)(offset & 0xFFFFFFFF);	

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
