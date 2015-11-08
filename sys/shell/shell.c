/**
 * Shell interpreter
 *
 * Copyright (C) 2009, Freie Universitaet Berlin (FUB).
 * Copyright (C) 2013, INRIA.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     shell
 * @{
 */

/**
 * @file
 * @brief       Implementation of a very simple command interpreter.
 *              For each command (i.e. "echo"), a handler can be specified.
 *              If the first word of a user-entered command line matches the
 *              name of a handler, the handler will be called with the whole
 *              command line as parameter.
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      René Kijewski <rene.kijewski@fu-berlin.de>
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#if USE_BETTER_CLI == 1
#include <unistd.h>
#include <termios.h>
#endif
#include "shell.h"
#include "shell_commands.h"

/*
 * See $(RIOT)/Makefile.include for USE_BETTER_CLI
 */
#if USE_BETTER_CLI == 1
#ifndef SHELL_HISTORY_SIZE
#define SHELL_HISTORY_SIZE  3
#endif
extern ssize_t _native_read(int fd, void *buf, size_t count);
#endif

static shell_command_handler_t find_handler(const shell_command_t *command_list, char *command)
{
    const shell_command_t *command_lists[] = {
        command_list,
#ifdef MODULE_SHELL_COMMANDS
        _shell_command_list,
#endif
    };

    const shell_command_t *entry;

    /* iterating over command_lists */
    for (unsigned int i = 0; i < sizeof(command_lists) / sizeof(entry); i++) {
        if ((entry = command_lists[i])) {
            /* iterating over commands in command_lists entry */
            while (entry->name != NULL) {
                if (strcmp(entry->name, command) == 0) {
                    return entry->handler;
                }
                else {
                    entry++;
                }
            }
        }
    }

    return NULL;
}

static void print_help(const shell_command_t *command_list)
{
    printf("%-20s %s\n", "Command", "Description");
    puts("---------------------------------------");

    const shell_command_t *command_lists[] = {
        command_list,
#ifdef MODULE_SHELL_COMMANDS
        _shell_command_list,
#endif
    };

    const shell_command_t *entry;

    /* iterating over command_lists */
    for (unsigned int i = 0; i < sizeof(command_lists) / sizeof(entry); i++) {
        if ((entry = command_lists[i])) {
            /* iterating over commands in command_lists entry */
            while (entry->name != NULL) {
                printf("%-20s %s\n", entry->name, entry->desc);
                entry++;
            }
        }
    }
}

static void handle_input_line(shell_t *shell, char *line)
{
    static const char *INCORRECT_QUOTING = "shell: incorrect quoting";

    /* first we need to calculate the number of arguments */
    unsigned argc = 0;
    char *pos = line;
    int contains_esc_seq = 0;
    while (1) {
        if ((unsigned char) *pos > ' ') {
            /* found an argument */
            if (*pos == '"' || *pos == '\'') {
                /* it's a quoted argument */
                const char quote_char = *pos;
                do {
                    ++pos;
                    if (!*pos) {
                        puts(INCORRECT_QUOTING);
                        return;
                    }
                    else if (*pos == '\\') {
                        /* skip over the next character */
                        ++contains_esc_seq;
                        ++pos;
                        if (!*pos) {
                            puts(INCORRECT_QUOTING);
                            return;
                        }
                        continue;
                    }
                } while (*pos != quote_char);
                if ((unsigned char) pos[1] > ' ') {
                    puts(INCORRECT_QUOTING);
                    return;
                }
            }
            else {
                /* it's an unquoted argument */
                do {
                    if (*pos == '\\') {
                        /* skip over the next character */
                        ++contains_esc_seq;
                        ++pos;
                        if (!*pos) {
                            puts(INCORRECT_QUOTING);
                            return;
                        }
                    }
                    ++pos;
                    if (*pos == '"') {
                        puts(INCORRECT_QUOTING);
                        return;
                    }
                } while ((unsigned char) *pos > ' ');
            }

            /* count the number of arguments we got */
            ++argc;
        }

        /* zero out the current position (space or quotation mark) and advance */
        if (*pos > 0) {
            *pos = 0;
            ++pos;
        }
        else {
            break;
        }
    }
    if (!argc) {
        return;
    }

    /* then we fill the argv array */
    char *argv[argc + 1];
    argv[argc] = NULL;
    pos = line;
    for (unsigned i = 0; i < argc; ++i) {
        while (!*pos) {
            ++pos;
        }
        if (*pos == '"' || *pos == '\'') {
            ++pos;
        }
        argv[i] = pos;
        while (*pos) {
            ++pos;
        }
    }
    for (char **arg = argv; contains_esc_seq && *arg; ++arg) {
        for (char *c = *arg; *c; ++c) {
            if (*c != '\\') {
                continue;
            }
            for (char *d = c; *d; ++d) {
                *d = d[1];
            }
            if (--contains_esc_seq == 0) {
                break;
            }
        }
    }

    /* then we call the appropriate handler */
    shell_command_handler_t handler = find_handler(shell->command_list, argv[0]);
    if (handler != NULL) {
        handler(argc, argv);
    }
    else {
        if (strcmp("help", argv[0]) == 0) {
            print_help(shell->command_list);
        }
        else {
            puts("shell: command not found:");
            puts(argv[0]);
        }
    }
}

#if USE_BETTER_CLI == 1
static void del_char(shell_t *shell)
{
    /* white-tape the character */
    shell->put_char('\b');
    shell->put_char(' ');
    shell->put_char('\b');
}

static void kill_readline(shell_t *shell, char *buf, char **line_buf_ptr)
{
    while (*line_buf_ptr > buf) {
        (*line_buf_ptr)--;
        del_char(shell);
    }
}

static int set_stdin_nonblock(void)
{
    struct termios ts;

    if (tcgetattr(STDIN_FILENO, &ts) < 0)
        return -1;

    ts.c_lflag &= ~(ICANON | ECHO);
    ts.c_cc[VMIN] = 1;

    tcsetattr(STDIN_FILENO, TCSANOW, &ts);

    return 0;
}

static int nonblock_getchar(void)
{
    unsigned char cmdbuf;

    if (_native_read(STDIN_FILENO, (void *)&cmdbuf, 1) <= 0)
        return -1;

    return (int)cmdbuf;
}

/*
 * tiny history feature
 */
struct hist_ctx {
    char **hist;
    int hist_current_index; /* the place for the next input */
    int hist_search_index;  /* point to the index during searching */
    int hist_max_index;
    shell_t *shell;
};
static struct hist_ctx *hist;

static void hist_init(shell_t *shell)
{
    int i = 0;

    if ((hist = malloc(sizeof(*hist))) == NULL)
            return;
    if ((hist->hist = malloc(sizeof(char *) * SHELL_HISTORY_SIZE)) == NULL) {
err:
        free(hist);
        hist = NULL;
        return;
    }
    for (i = 0; i < SHELL_HISTORY_SIZE; i++) {
        if ((hist->hist[i] = malloc(shell->shell_buffer_size)) == NULL) {
            while (i--) {
                if (hist->hist[i] != NULL)
                    free(hist->hist[i]);
            }
            goto err;
        }
        hist->hist[i][0] = '\0';
    }
    hist->hist_current_index = 0;
    hist->hist_search_index = -1;
    hist->hist_max_index = -1;
    hist->shell = shell;
    return;
}

static int
hist_prev_index(void)
{
    if (hist->hist_current_index == 0)
        return hist->hist_max_index;
    return hist->hist_current_index - 1;
}

/*
 * don't need to check the length of the command line is less than the buffer size.
 * it has been done in the readline() loop.
 */
static void hist_put(char *line_buf)
{
    /*
     * check whether the command is exactly identical to the previous command.
     * if so, it doesn't put the history.
     */
    if (hist->hist_max_index != -1 &&
            strcmp(hist->hist[hist_prev_index()], line_buf) == 0) {
        return;
    }

    snprintf(hist->hist[hist->hist_current_index], hist->shell->shell_buffer_size,
            "%s", line_buf);

    if (hist->hist_current_index + 1 < SHELL_HISTORY_SIZE)
        hist->hist_current_index++;
    else
        hist->hist_current_index = 0;

    if (hist->hist_max_index + 1 < SHELL_HISTORY_SIZE)
        hist->hist_max_index++;

    hist->hist_search_index = -1;
}

static void hist_get(int index, char *line_buf, char **line_buf_ptr)
{
    int len, i;

    kill_readline(hist->shell, line_buf, line_buf_ptr);

    /*
     * no need to check whether the line_buf size is enough to copy the command,
     * because it is checked when a command was copied into the history.
     */
    len = strlen(hist->hist[index]);
    for (i = 0; i < len; i++) {
        hist->shell->put_char(hist->hist[index][i]);
        line_buf[i] = hist->hist[index][i];
    }

    *line_buf_ptr = line_buf + len;
}

static void hist_prev(char *line_buf, char **line_buf_ptr)
{
    /* no history so far */
    if (hist->hist_max_index == -1)
        return;

    if (hist->hist_search_index == -1) {
        /* just enter searching the history */
        hist->hist_search_index = hist_prev_index();
    } else {
        /* in searching */
        if (hist->hist_search_index == hist->hist_current_index)
            return; /* reached the head of the history */
        else if (hist->hist_search_index == 0) {
            if (hist->hist_max_index < SHELL_HISTORY_SIZE - 1)
                return;
            hist->hist_search_index = hist->hist_max_index;
        } else
            hist->hist_search_index--;
    }

    hist_get(hist->hist_search_index, line_buf, line_buf_ptr);
}

static void hist_next(char *line_buf, char **line_buf_ptr)
{
    /* no history so far */
    if (hist->hist_search_index == -1)
        return;

    /* end of searching the history */
    if (hist->hist_search_index + 1 == hist->hist_current_index ||
            (hist->hist_current_index == 0 && hist->hist_search_index == hist->hist_max_index)) {
        kill_readline(hist->shell, line_buf, line_buf_ptr);
        hist->hist_search_index = -1;
        return;
    }

    if (hist->hist_search_index + 1 < SHELL_HISTORY_SIZE &&
            hist->hist_search_index + 1 != hist->hist_current_index)
        hist->hist_search_index++;
    else if (hist->hist_current_index != 0)
        hist->hist_search_index = 0;
    else
        return;

    hist_get(hist->hist_search_index, line_buf, line_buf_ptr);
}
#endif /* USE_BETTER_CLI == 1 */

static int readline(shell_t *shell, char *buf, size_t size)
{
    char *line_buf_ptr = buf;
#if USE_BETTER_CLI == 1
     int escaped = 0;
#endif

    while (1) {
        if ((line_buf_ptr - buf) >= ((int) size) - 1) {
            return -1;
        }

#if USE_BETTER_CLI == 1
        int c = nonblock_getchar();
#else
        int c = shell->readchar();
#endif
        if (c < 0) {
            return 1;
        }

        /* We allow Unix linebreaks (\n), DOS linebreaks (\r\n), and Mac linebreaks (\r). */
        /* QEMU transmits only a single '\r' == 13 on hitting enter ("-serial stdio"). */
        /* DOS newlines are handled like hitting enter twice, but empty lines are ignored. */
        if (c == '\r' || c == '\n') {
            if (line_buf_ptr == buf) {
                /* The line is empty. */
                continue;
            }

            *line_buf_ptr = '\0';
#if USE_BETTER_CLI == 1
            hist_put(buf);
#endif
            shell->put_char('\r');
            shell->put_char('\n');
            return 0;
        }
#if !defined(USE_BETTER_CLI) || USE_BETTER_CLI == 0
        /* QEMU uses 0x7f (DEL) as backspace, while 0x08 (BS) is for most terminals */
        else if (c == 0x08 || c == 0x7f) {
            if (line_buf_ptr == buf) {
                /* The line is empty. */
                continue;
            }

            *--line_buf_ptr = '\0';
            /* white-tape the character */
            shell->put_char('\b');
            shell->put_char(' ');
            shell->put_char('\b');
        }
#else
        /* QEMU uses 0x7f (DEL) as backspace, while 0x08 (BS) is for most terminals */
        else if (c == 0x08 || c == 0x7f || c == 0x3f) {
            if (line_buf_ptr == buf) {
                /* The line is empty. */
                continue;
            }

            *--line_buf_ptr = '\0';
            /* white-tape the character */
            del_char(shell);
        }
        else if (c == 0x15) {
            /* kill the command line */
            /* ^U */
            kill_readline(shell, buf, &line_buf_ptr);
        }
        else if (c == 0x10 || (escaped == 2 && c == 0x41)) {
            /* ^P or ^[[A */
            hist_prev(buf, &line_buf_ptr);
            escaped = 0;
        }
        else if (c == 0xe || (escaped == 2 && c == 0x42)) {
            /* ^N or ^[[B */
            hist_next(buf, &line_buf_ptr);
            escaped = 0;
        }
        else if (c == 0x1b) {
            escaped = 1;
        }
        else if (escaped == 1 && c == 0x5b) {
            escaped = 2;
        }
#endif
        else {
            *line_buf_ptr++ = c;
            shell->put_char(c);
        }
    }
}

static inline void print_prompt(shell_t *shell)
{
    shell->put_char('>');
    shell->put_char(' ');
    return;
}

void shell_run(shell_t *shell)
{
    char line_buf[shell->shell_buffer_size];

#if USE_BETTER_CLI == 1
    set_stdin_nonblock();
    hist_init(shell);
#endif

    print_prompt(shell);

    while (1) {
        int res = readline(shell, line_buf, sizeof(line_buf));

        if (!res) {
            handle_input_line(shell, line_buf);
        }

        print_prompt(shell);
    }
}

void shell_init(shell_t *shell, const shell_command_t *shell_commands,
                uint16_t shell_buffer_size, int(*readchar)(void), void(*put_char)(int))
{
    shell->command_list = shell_commands;
    shell->shell_buffer_size = shell_buffer_size;
    shell->readchar = readchar;
    shell->put_char = put_char;
}

/** @} */
