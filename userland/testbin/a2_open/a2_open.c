#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>

#define MAX_BUF 500
char teststr1[] = "HahaHehe";
char teststr2[] = "Haha";
char buf[MAX_BUF];

int
main(int argc, char * argv[])
{

/*
O_RDONLY		Open for reading only.
O_WRONLY		Open for writing only.
O_RDWR		    Open for reading and writing.

O_CREAT 	    Create the file if it doesn't exist.
O_EXCL		    Fail if the file already exists.
O_TRUNC	 c	    Truncate the file to length 0 upon open.
O_APPEND c 	    Open the file in append mode.



ENODEV 	        The device prefix of filename did not exist.
ENOTDIR	 	    A non-final component of filename was not a directory.
ENOENT		    A non-final component of filename did not exist.
ENOENT		    The named file does not exist, and O_CREAT was not specified.
EEXIST		    The named file exists, and O_EXCL was specified.
EISDIR		    The named object is a directory, and it was to be opened for writing.
EMFILE		    The process's file table was full, or a process-specific limit on open files was reached.
ENFILE   c		The system file table is full, if such a thing exists, or a system-wide limit on open files was reached.
ENXIO		    The named object is a block device with no filesystem mounted on it.
ENOSPC		    The file was to be created, and the filesystem involved is full.
EINVAL		    flags contained invalid values.
EIO		        A hard I/O error occurred.
EFAULT		    filename was an invalid pointer.



Error:

write syscall in printf, %s return 6
O_APPEND error


*/

        int fd, r;
        (void) argc;
        (void) argv;

        printf("*********** open test ***********");

/*
		printf("*********** file descriptor maximum test *********\n");
		while(1){
			fd = open("test.file", O_RDWR | O_CREAT );
			printf("* open() got fd %d\n", fd);
			if (fd < 0) {
				printf("ERROR opening file: %s\n", strerror(errno));
				break;
			}
		}

*/


 		// sys161 kernel "p /testbin/a2_test"


		// *************** open *********************
        printf("* testing open\n");
		fd = open("test.file", O_RDWR | O_CREAT );
		printf("* open() got fd %d\n", fd);
		printf("* end open\n\n");
		// ************* end open *********************
		// *************** write *********************
		str = teststr1;
        printf("* testing write: %s\n", str);
        r = write(fd, str, strlen(str));
        printf("* wrote %d bytes\n", r);
        if (r < 0) {
                printf("ERROR writing file: %s\n", strerror(errno));
                exit(1);
        }
		printf("* end write\n\n");
		// ************* end write *********************
		// *************** close *********************
        printf("* testing close \n");
		close(fd);
		printf("* end close\n\n");
		// *************** end close *********************






		// *************** open *********************
        printf("* testing open\n");
		fd = open("test.file", O_RDWR | O_APPEND );
		printf("* open() got fd %d\n", fd);
		printf("* end open\n\n");
		// ************* end open *********************
		// *************** write *********************
		str = teststr2;
        printf("* testing write: %s\n", str);
        r = write(fd, str, strlen(str));
        printf("* wrote %d bytes\n", r);
        if (r < 0) {
                printf("ERROR writing file: %s\n", strerror(errno));
                exit(1);
        }
		printf("* end write\n\n");
		// ************* end write *********************
		// *************** close *********************
        printf("* testing close \n");
		close(fd);
		printf("* end close\n\n");
		// *************** end close *********************








        return 0;
}
