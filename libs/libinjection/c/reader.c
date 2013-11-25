#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libinjection.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static int g_test_ok = 0;
static int g_test_fail = 0;

int urlcharmap(char ch) {
    switch (ch) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default:
        return 256;
    }
}

size_t modp_url_decode(char* dest, const char* s, size_t len)
{
    const char* deststart = dest;

    size_t i = 0;
    int d = 0;
    while (i < len) {
        switch (s[i]) {
        case '+':
            *dest++ = ' ';
            i += 1;
            break;
        case '%':
            if (i+2 < len) {
                d = (urlcharmap(s[i+1]) << 4) | urlcharmap(s[i+2]);
                if ( d < 256) {
                    *dest = (char) d;
                    dest++;
                    i += 3; /* loop will increment one time */
                } else {
                    *dest++ = '%';
                    i += 1;
                }
            } else {
                *dest++ = '%';
                i += 1;
            }
            break;
        default:
            *dest++ = s[i];
            i += 1;
        }
    }
    *dest = '\0';
    return (size_t)(dest - deststart); // compute "strlen" of dest.
}

void modp_toprint(char* str, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        if (str[i] < 32 || str[i] > 126) {
            str[i] = '?';
        }
    }
}
size_t modp_rtrim(char* str, size_t len)
{
    while (len) {
        char c = str[len -1];
        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
            str[len -1] = '\0';
            len -= 1;
        } else {
            break;
        }
    }
    return len;
}

void test_positive(FILE * fd, const char *fname,
                   int flag_invert, int flag_true, int flag_quiet)
{
    char linebuf[8192];
    int issqli;
    int linenum = 0;
    sfilter sf;

    while (fgets(linebuf, sizeof(linebuf), fd)) {
        linenum += 1;
        size_t len = modp_rtrim(linebuf, strlen(linebuf));
        if (len == 0) {
            continue;
        }
        if (linebuf[0] == '#') {
            continue;
        }

        len =  modp_url_decode(linebuf, linebuf, len);
        libinjection_sqli_init(&sf, linebuf, len, 0);
        issqli = libinjection_is_sqli(&sf);
        if (issqli) {
            g_test_ok += 1;
        } else {
            g_test_fail += 1;
        }

        if (!flag_quiet) {
            if ((issqli && flag_true && ! flag_invert) ||
                (!issqli && flag_true && flag_invert) ||
                !flag_true) {

                // if we didn't find a SQLi and fingerprint from sqlstats is
                // is 'sns' or 'snsns' then redo using plain context
                if (!issqli && (strcmp(sf.fingerprint, "sns") == 0 || strcmp(sf.fingerprint, "snsns") == 0)) {
                    libinjection_sqli_fingerprint(&sf, 0);
                }
                modp_toprint(linebuf, len);
                fprintf(stdout, "%s\t%d\t%s\t%s\t%s\n",
                        fname, linenum,
                        (issqli ? "True" : "False"), sf.fingerprint, linebuf);
            }
        }
    }
}

int main(int argc, const char *argv[])
{
    /*
     * invert output, by
     */
    int flag_invert = FALSE;

    /*
     * don't print anything.. useful for
     * performance monitors, gprof.
     */
    int flag_quiet = FALSE;

    /*
     * only print postive results
     * with invert, only print negative results
     */
    int flag_true = FALSE;

    int flag_slow = 1;
    int count = 0;
    int max = -1;

    int i, j;
    int offset = 1;

    while (offset < argc) {
        if (strcmp(argv[offset], "-i") == 0) {
            offset += 1;
            flag_invert = TRUE;
        } else if (strcmp(argv[offset], "-q") == 0) {
            offset += 1;
            flag_quiet = TRUE;
        } else if (strcmp(argv[offset], "-t") == 0) {
            offset += 1;
            flag_true = TRUE;
        } else if (strcmp(argv[offset], "-s") == 0) {
            offset += 1;
            flag_slow = 100;
        } else if (strcmp(argv[offset], "-m") == 0) {
            offset += 1;
            max = atoi(argv[offset]);
            offset += 1;
        } else {
            break;
        }
    }

    if (offset == argc) {
        test_positive(stdin, "stdin", flag_invert, flag_true, flag_quiet);
    } else {
        for (j = 0; j < flag_slow; ++j) {
            for (i = offset; i < argc; ++i) {
                FILE* fd = fopen(argv[i], "r");
                if (fd) {
                    test_positive(fd, argv[i], flag_invert, flag_true, flag_quiet);
                    fclose(fd);
                }
            }
        }
    }

    if (!flag_quiet) {
        fprintf(stdout, "%s", "\n");
        fprintf(stdout, "SQLI  : %d\n", g_test_ok);
        fprintf(stdout, "SAFE  : %d\n", g_test_fail);
        fprintf(stdout, "TOTAL : %d\n", g_test_ok + g_test_fail);
    }

    if (max == -1) {
        return 0;
    }

    count = g_test_ok;
    if (flag_invert) {
        count = g_test_fail;
    }

    if (count > max) {
        printf("\nTheshold is %d, got %d, failing.\n", max, count);
        return 1;
    } else {
        printf("\nTheshold is %d, got %d, passing.\n", max, count);
        return 0;
    }
}