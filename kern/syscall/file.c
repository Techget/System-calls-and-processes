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


/*
 * Add your file-related functions here ...
 */


int 
sys_open(const char *filename, int flags, mode_t mode, int *retval){
	kprintf("open(%s, %d, %d)\n", filename, flags, mode);
	*retval = 0;
/*
	int result = 0;
	int index = 3;

	if(!(flags==O_RDONLY || flags==O_WRONLY || flags==O_RDWR || flags==(O_RDWR|O_CREAT|O_TRUNC))) {
		return EINVAL;
	}


	char *kbuf;
	kbuf = (char *)kmalloc(sizeof(char)*PATH_MAX);

	result = copyinstr((const_userptr_t)filename,kbuf, PATH_MAX, &len);
	if(result) {
		kfree(kbuf);
		return result;
	}
*/

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

void opf_table_init(void){
	// set to null etc.

	// vnode stdin

	
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
	// stdout
	curproc->p_fdtable->fdt[1] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[1]->open_file = open_file_table[1];
	curproc->p_fdtable->fdt[1]->offset = 0;
	// stderr
	curproc->p_fdtable->fdt[2] = (struct fd *)kmalloc(sizeof(struct fd));
	curproc->p_fdtable->fdt[2]->open_file = open_file_table[2];
	curproc->p_fdtable->fdt[2]->offset = 0;
}
