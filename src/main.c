#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define CLEAR "\033[2J"
#define RESET_CURSOR "\033[H"

int set_cursor_pos(const int fd, const int row, const int col)
{
        const int MAX_SIZE = 256;
        char command[MAX_SIZE];
        int res = snprintf(command, MAX_SIZE, "\033[%d;%dH", row, col);
        if (res < 0 || res > MAX_SIZE)
        {
                perror("error");
                return -1;
        }

        write(fd, command, strlen(command));

        return 0;
}


int main(int argc, char* argv[])
{
        char buf[BUFSIZ] = {0};
        setbuf(stdin, buf);

        struct winsize window;

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == -1)
        {
                perror("error");
                return -1;
        }
        int num_rows = window.ws_row;
        int num_cols = window.ws_col;

        char* tui = malloc(num_rows*num_cols + 256);
        if (tui == NULL)
        {
                perror("error");
                return -1;
        }

        const char* test = CLEAR RESET_CURSOR "test\n";
        strcpy(tui, test);
        int offset = strlen(test);
        memset(tui + offset, '\n', num_rows - 2);
        tui[num_rows + offset - 2] = '\0';

        int fd = open("/dev/tty", O_WRONLY);
        if (fd == -1)
        {
                perror("error");
                return -1;
        }

        write(fd, tui, strlen(tui));
        set_cursor_pos(fd, 10, 5);

        getchar();

        close(fd);
        free(tui);

        return 0;
}
