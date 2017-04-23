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
sys_open(const char *filename, int flags, mode_t mode){
	kprintf("open(%s, %d, %d)\n", filename, flags, mode);
		return 0;
}

int 
sys_dup2(int oldfd, int newfd){
	kprintf("dup2(%d, %d)\n", oldfd, newfd);
	return 0;
}


int 
sys_close(int fd){
	kprintf("close(%d )\n", fd);
	return 0;
}


int 
sys_read(int fd, void *buf, size_t count){
	kprintf("read(%d, -, %d)\n", fd, count);
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf)*count);

	kfree(kbuf);

	return 0;
}


int 
sys_write(int fd, const void *buf, size_t count){
	kprintf("write(%d, -, %d)\n", fd, count);
	void *kbuf;
	kbuf = kmalloc(sizeof(*buf)*count);

	kfree(kbuf);
	return 0;

}


off_t 
sys_lseek(int fd, off_t offset, int whence){
	kprintf("lseek(%d, - , %d)\n", fd, whence);
	void *kbuf;
	kbuf = kmalloc(sizeof(offset));

	kfree(kbuf);
	return 0;
}

