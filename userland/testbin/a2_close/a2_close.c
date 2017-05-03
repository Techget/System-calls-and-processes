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


*/

        int fd, r;
		char *str;
        (void) argc;
        (void) argv;

        printf("*********** lseek test ***********");




		// *************** open *********************
		fd = open("test.file", O_RDWR | O_CREAT );
		printf("* open() got fd %d\n", fd);
		// ************* end open *********************




		// *************** write *********************
		str = teststr1;
        printf("* writing test string: %s\n", str);
        r = write(fd, str, strlen(str));
        printf("* wrote %d bytes\n", r);
        if (r < 0) {
                printf("ERROR writing file: %s\n", strerror(errno));
                exit(1);
        }
		// ************* end write *********************




		// *************** close *********************
		close(fd);
		// *************** end close *********************




		fd = open("test.file", O_RDWR|O_TRUNC);
		printf("* open() with TRUNC got fd %d\n", fd);
		if (fd < 0) {
			printf("ERROR opening file: %s\n", strerror(errno));
			exit(1);
		}


        printf("* writing test string\n");
        r = write(fd, teststr2, strlen(teststr2));
        printf("* wrote %d bytes\n", r);
        if (r < 0) {
                printf("ERROR writing file: %s\n", strerror(errno));
                exit(1);
        }

		close(fd);




        return 0;
}
