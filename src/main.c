#include <stdio.h>
#include <stdlib.h>

#include "amiga_runtime.h"

int main(int argc, char **argv)
{
    unsigned long port = 23u;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: RexxTelnet host [port]\n");
        return 5;
    }

    if (argc == 3) {
        char *end = NULL;
        port = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || port == 0u || port > 65535u) {
            fprintf(stderr, "RexxTelnet: invalid port\n");
            return 5;
        }
    }

    return rt_amiga_run(argv[1], (unsigned short)port);
}
