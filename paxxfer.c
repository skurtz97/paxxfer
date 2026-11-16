/* paxxfer - copy directory trees between z/OS UNIX and Linux over ssh,
 * converting text code pages.  Default direction is pull: the operand
 * that looks like [user@]host:path is the remote side. */

#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__GNUC__)
#define PRINTF_LIKE(f, a) __attribute__((format(printf, f, a)))
#else
#define PRINTF_LIKE(f, a)
#endif

static const char *prog = "paxxfer";

_Noreturn PRINTF_LIKE(1, 2) static void die(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "%s: ", prog);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/* Contract: returns a malloc'd copy of s[0..n); dies on OOM. */
static char *xstrndup(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (p == NULL)
        die("out of memory");
    memcpy(p, s, n); /* s must have >= n readable bytes */
    p[n] = '\0';
    return p;
}

typedef enum direction { DIR_PULL, DIR_PUSH } direction;

typedef struct remote {
    char *user; /* NULL if not given */
    char *host;
    char *path;
} remote;

typedef struct opts {
    bool verbose, dryrun, force;
    const char *acs; /* NULL = detect */
    const char *ecs; /* NULL = detect */
    direction dir;
    remote rem;
    const char *local; /* points into argv */
} opts;

PRINTF_LIKE(2, 3) static void vlog(const opts *o, const char *fmt, ...) {
    va_list ap;
    if (!o->verbose)
        return;
    fprintf(stderr, "%s: ", prog);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

_Noreturn static void usage(void) {
    fprintf(stderr,
            "usage: %s [-nvF] [-a ascii_cs] [-e ebcdic_cs] src dst\n"
            "  pull: %s [user@]host:srcdir localdir\n"
            "  push: %s localdir [user@]host:dstdir\n"
            "  (prefix a local path containing ':' with ./)\n",
            prog, prog, prog);
    exit(2);
}

static void parse_args(int argc, char **argv, opts *o) {
    remote r0 = {0}, r1 = {0};
    bool rem0, rem1;
    int c;

    if (argc > 0 && argv[0] != NULL && argv[0][0] != '\0') {
        const char *b = strrchr(argv[0], '/');
        prog = b != NULL ? b + 1 : argv[0];
    }

    while ((c = getopt(argc, argv, "a:e:nvF")) != -1) {
        switch (c) {
        case 'a':
            o->acs = optarg;
            break;
        case 'e':
            o->ecs = optarg;
            break;
        case 'n':
            o->dryrun = true;
            break;
        case 'v':
            o->verbose = true;
            break;
        case 'F':
            o->force = true;
            break;
        default:
            usage();
        }
    }
    if (argc - optind != 2)
        usage();

    rem0 = parse_remote(argv[optind], &r0);
    rem1 = parse_remote(argv[optind + 1], &r1);
    if (rem0 == rem1)
        die("exactly one operand must be [user@]host:path");

    if (rem0) {
        o->dir = DIR_PULL;
        o->rem = r0;
        o->local = argv[optind + 1];
    } else {
        o->dir = DIR_PUSH;
        o->rem = r1;
        o->local = argv[optind];
    }
}