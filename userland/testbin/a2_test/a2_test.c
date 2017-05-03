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

        printf("*********** test template ***********");

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









		// *************** read *********************
        printf("* testing read \n");
        int i = 0;
        do  {
                printf("* attemping read of %d bytes\n", MAX_BUF -i);
                r = read(fd, &buf[i], MAX_BUF - i);
                printf("* read %d bytes\n", r);
                i += r;
        } while (i < MAX_BUF && r > 0);

        if (r < 0) {
                printf("ERROR reading file: %s\n", strerror(errno));
                exit(1);
        }
        printf("* read: %s\n", buf);
		printf("* end read\n\n");
		// *************** end read *********************









		// *************** lseek *********************
 		printf("* testing lseek\n");
        r = lseek(fd, 5, SEEK_SET);
        if (r < 0) {
                printf("ERROR lseek: %s\n", strerror(errno));
                exit(1);
        }
		printf("* end lseek\n\n");
		// *************** end lseek *********************







		// *************** dup2 *********************
 		printf("* testing dup2\n");
        r = dup2(fd, 0);
        if (r < 0) {
                printf("ERROR dup2: %s\n", strerror(errno));
                exit(1);
        }
		printf("* end dup2\n\n");
		// *************** end dup2 *********************








		// *************** close *********************
        printf("* testing close \n");
		close(fd);
		printf("* end close\n\n");
		// *************** end close *********************






        return 0;
}
