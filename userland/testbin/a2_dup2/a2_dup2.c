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
        (void) argc;
        (void) argv;

        printf("*********** dup2 test ***********");




		// *************** open *********************
        printf("* testing open\n");
		fd = open("test.file", O_RDWR | O_CREAT );
		printf("* open() got fd %d\n", fd);
		printf("* end open\n\n");
		// ************* end open *********************




		// *************** dup2 *********************
 		printf("* testing dup2\n");
        r = dup2(fd, 1);
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
