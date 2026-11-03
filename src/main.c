#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define clear "\033[2j"
#define reset_cursor "\033[h"

int main(int argc, char* argv[])
{
        struct winsize window;

        if (ioctl(stdout_fileno, tiocgwinsz, &window) == -1)
        {
                perror("error");
                return -1;
        }
        int num_rows = window.ws_row;
        int num_cols = window.ws_col;

        char* tui = malloc(num_rows*num_cols + 256);
        if (tui == null)
        {
                perror("error");
                return -1;
        }

        const char* test = clear reset_cursor "test\n";
        strcpy(tui, test);
        int offset = strlen(test);
        memset(tui + offset, '\n', num_rows-2);
        tui[num_rows + offset - 1] = '\0';

        int fd = open("/dev/tty", o_wronly);
        if (fd == -1)
        {
                perror("error");
                return -1;
        }

        write(fd, tui, strlen(tui));

        close(fd);
        free(tui);

        getchar();

        return 0;
}
