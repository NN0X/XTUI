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

typedef struct Line
{
        char* data;
        int len;

        struct Line* prev_line;
        struct Line* next_line;
} Line;

typedef struct TUI
{
        Line* first_line;
        Line* last_line;
        int num_lines;

        int num_rows;
        int num_cols;

        Line* cur_line;
        int cur_top_offset;
        int cur_col;
} TUI;

int create_tui(TUI* tui, int num_rows, int num_cols)
{
        *tui = (TUI){
                .first_line = NULL,
                .last_line = NULL,
                .num_lines = 0,

                .num_rows = num_rows,
                .num_cols = num_cols,
                .cur_line = NULL,
                .cur_top_offset = 0,
                .cur_col = 0
        };

        return 0;
}

int populate_tui(TUI* tui, Line* first_line)
{
        tui->first_line = first_line;

        Line *curr_line = first_line;
        while (curr_line->next_line != NULL)
        {
                curr_line = curr_line->next_line;
                tui->num_lines++;
        }

        tui->last_line = curr_line;

        return 0;
}

int gen_tui_buf(TUI* tui, char* buf)
{
        return 0;
}

int update_tui_cur(TUI* tui, const int fd)
{
        set_cursor_pos(fd, 0, 0);

        return 0;
}

int create_line(Line* line, const char* data, int len, Line* prev_line)
{
        *line = (Line){
                .data = NULL,
                .len = len,
                .prev_line = prev_line,
                .next_line = NULL
        };

        if (line->prev_line != NULL) { line->prev_line->next_line = line; };

        line->data = malloc(len);
        if (line->data == NULL)
        {
                perror("error");
                return 1;
        }
        memcpy(line->data, data, len);

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

        TUI tui;
        create_tui(&tui, window.ws_row, window.ws_col);

        Line first_line;
        const char* test = "test";
        create_line(&first_line, test, strlen(test), NULL);

        Line* prev_line = &first_line;
        for (int i = 1; i < tui.num_rows; i++)
        {
                Line* line = malloc(sizeof(Line));
                if (line == NULL)
                {
                        perror("error");
                        return 1;
                }
                create_line(line, "", 0, prev_line);
                prev_line = line;
        }

        populate_tui(&tui, &first_line);

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
                return 1;
        }

        char* tui_buf;
        if (gen_tui_buf(&tui, tui_buf) == 1)
        {
                return 1;
        }
        write(fd, tui_buf, strlen(tui_buf));
        update_tui_cur(&tui, fd);

        if (arm_sigio(fd) == 1)
        {
                tcsetattr(fd, TCSAFLUSH, &orig);
                close(fd);
                return 1;
        }

        for(;;)
        {
                pause();
        }

        tcsetattr(fd, TCSAFLUSH, &orig);
        close(fd);

        return 0;
}
