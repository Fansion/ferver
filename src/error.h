#ifndef __ERROR_H__
#define __ERROR_H__

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    /* exit(0); */
}

void posix_error(int code, char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    /* exit(0); */
}

#endif
