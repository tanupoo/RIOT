/**
 * Native CPU entry code
 *
 * Copyright (C) 2013 Ludwig Ortmann
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup arch
 * @{
 * @file
 * @author  Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 * @}
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <dlfcn.h>
#else
#include <dlfcn.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <kernel_internal.h>
#include <cpu.h>

#include "kernel_internal.h"
#include "cpu.h"

#include "board_internal.h"
#include "native_internal.h"
#include "tap.h"

int (*real_printf)(const char *format, ...);
char *argv0;
int _native_null_in_pipe[2];
int _native_null_out_file;

void daemonize()
{
    pid_t pid;

    /* fork */
    if ((pid = fork()) == -1) {
        err(EXIT_FAILURE, "daemonize: fork");
    }

    if (pid > 0) {
        real_printf("RIOT pid: %d\n", pid);
        exit(EXIT_SUCCESS);
    }

}

void usage_exit()
{
    real_printf("usage: %s", argv0);
#ifdef MODULE_NATIVENET
    real_printf(" <tap interface>");
#endif
    real_printf(" [-t <port>|-u]");
    real_printf(" [-d] [-e]\n");

    real_printf("\nOptions:\n\
-d      daeomize\n");
    real_printf("\
-u      redirect stdio to unix socket\n\
-t      redirect stdio to tcp socket\n\
-e      redirect stderr file\n\
-o      redirect stdout to file when not attached to terminal\n");
    real_printf("\n\
The order of command line arguments matters.\n");
    exit(EXIT_FAILURE);
}

__attribute__((constructor)) static void startup(int argc, char **argv)
{
    /* get system read/write/printf */
    *(void **)(&real_read) = dlsym(RTLD_NEXT, "read");
    *(void **)(&real_write) = dlsym(RTLD_NEXT, "write");
    *(void **)(&real_printf) = dlsym(RTLD_NEXT, "printf");

    argv0 = argv[0];
    int argp = 1;
    char *stderrtype = "stdio";
    char *stdiotype = "stdio";
    char *nullouttype = NULL;
    char *ioparam = NULL;

#ifdef MODULE_NATIVENET
    if (argc < 2) {
        usage_exit();
    }
    argp++;
#endif
#ifdef MODULE_UART0
    for (; argp < argc; argp++) {
        char *arg = argv[argp];
        if (strcmp("-d", arg) == 0) {
            daemonize();
        }
        else if (strcmp("-e", arg) == 0) {
            stderrtype = "file";
        }
        else if (strcmp("-o", arg) == 0) {
            nullouttype = "file";
        }
        else if (strcmp("-t", arg) == 0) {
            stdiotype = "tcp";
            if (argp+1 < argc) {
                ioparam = argv[++argp];
            }
            else {
                usage_exit();
            }
        }
        else if (strcmp("-u", arg) == 0) {
            stdiotype = "unix";
        }
        else {
            usage_exit();
        }
    }
#endif


#ifdef MODULE_UART0
    _native_init_uart0(stdiotype, stderrtype, nullouttype, ioparam);
#endif

    native_hwtimer_pre_init();
    native_cpu_init();
    native_interrupt_init();
#ifdef MODULE_NATIVENET
    tap_init(argv[1]);
#endif

    board_init();

    puts("RIOT native hardware initialization complete.\n");
    kernel_init();
}
