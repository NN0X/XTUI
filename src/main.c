#define _GNU_SOURCE
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>

#define CLEAR "\033[2J"
#define RESET_CURSOR "\033[H"

static char buf[BUFSIZ] = {0};

static void on_sigio(int sig, siginfo_t *si, void *uc)
{
        ssize_t n = read(si->si_fd, buf, sizeof buf - 1);
        if (n > 0) buf[n] = '\0';
}

static int enter_raw_mode(int fd, struct termios *term)
{
        if (tcgetattr(fd, term) == -1)
        {
                perror("error");
                return 1;
        }
        struct termios raw = *term;
        cfmakeraw(&raw);
        if (tcsetattr(fd, TCSAFLUSH, &raw) == -1)
        {
                perror("error");
                return 1;
        }
        return 0;
}

static int arm_sigio(int fd)
{
        struct sigaction sa = {0};
        sa.sa_sigaction = on_sigio;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGRTMIN, &sa, NULL) == -1)
        {
                perror("error");
                return 1;
        }
        fcntl(fd, F_SETSIG, SIGRTMIN);
        fcntl(fd, F_SETOWN, getpid());
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC);
        return 0;
}

int set_cursor_pos(const int fd, const int row, const int col)
{
        const int MAX_SIZE = 256;
        char command[MAX_SIZE];
        int res = snprintf(command, MAX_SIZE, "\033[%d;%dH", row, col);
        if (res < 0 || res >= MAX_SIZE)
        {
                perror("error");
                return 1;
        }

        write(fd, command, strlen(command));

        return 0;
}

int main(int argc, char* argv[])
{
        setbuf(stdin, buf);

        struct winsize window;

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == -1)
        {
                perror("error");
                return 1;
        }
        int num_rows = window.ws_row;
        int num_cols = window.ws_col;

        char* tui = malloc(num_rows*num_cols + 256);
        if (tui == NULL)
        {
                perror("error");
                return 1;
        }

        const char* test = CLEAR RESET_CURSOR "test\n";
        strcpy(tui, test);
        int offset = strlen(test);
        memset(tui + offset, '\n', num_rows - 2);
        tui[num_rows + offset - 2] = '\0';

        int fd = open("/dev/tty", O_RDWR | O_NONBLOCK);
        if (fd == -1)
        {
                perror("error");
                return 1;
        }

        struct termios orig;
        if (enter_raw_mode(fd, &orig) == 1)
        {
                close(fd);
                free(tui);
                return 1;
        }

        write(fd, tui, strlen(tui));
        set_cursor_pos(fd, 10, 5);

        if (arm_sigio(fd) == 1)
        {
                tcsetattr(fd, TCSAFLUSH, &orig);
                close(fd);
                free(tui);
                return 1;
        }

        for(;;)
        {
                pause();
                if (buf[0] == 'q') break;
                write(fd, buf, strlen(buf));
        }

        tcsetattr(fd, TCSAFLUSH, &orig);
        close(fd);
        free(tui);

        return 0;
}
