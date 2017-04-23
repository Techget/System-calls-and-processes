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

		curproc->p_fdtable->fdt[index]->open_file = ofile;
		open_file_table[i] = ofile;
	}
	// link with existing open file table
	if(open_file_table[i]->vn == vn){
		open_file_table[i]->refcount++;
		curproc->p_fdtable->fdt[index]->open_file = open_file_table[i];
	}

	// set return value as file descriptor
	*retval = index;
	kfree(kbuf);

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
	kprintf("read(%d, -, %d)\n", fd, count);
	*retval = 0;
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf));

	kfree(kbuf);
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
	curproc->p_fdtable->fdt[0]->offset = 0;
	curproc->p_fdtable->fdt[0]->flags = O_RDONLY;
	// stdout
	curproc->p_fdtable->fdt[1] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[1]->open_file = open_file_table[1];
	curproc->p_fdtable->fdt[1]->offset = 0;
	curproc->p_fdtable->fdt[1]->flags = O_WRONLY;
	// stderr
	curproc->p_fdtable->fdt[2] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[2]->open_file = open_file_table[2];
	curproc->p_fdtable->fdt[2]->offset = 0;
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

	// set correponding field of struct opf
	opf_stdin->vn = vn_stdin;
	opf_stdout->vn = vn_stdout;
	opf_stderr->vn = vn_stderr;

	// set open_file_table[0,1,2] to stdin/out/err
	open_file_table[0] = opf_stdin;
	open_file_table[1] = opf_stdout;
	open_file_table[2] = opf_stderr;
}
