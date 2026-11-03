#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define CLEAR "\033[2J"
#define RESET_CURSOR "\033[H"

int main(int argc, char* argv[])
{
        const char* tui = CLEAR RESET_CURSOR "test\n";
        int fd = open("/dev/tty", O_WRONLY);
        if (fd == -1)
        {
                printf("ERROR\n");
                return -1;
        }

        write(fd, tui, strlen(tui));
        close(fd);

        return 0;
}
