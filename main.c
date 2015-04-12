#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include "pci_codes.h"

#define ALL_IO    3
#define RVAL_ERR -1

int increasePrivelegies(short level)
{
    if (iopl(level)) {
        perror("ERROR iopl:");
        return RVAL_ERR;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    increasePrivelegies(ALL_IO);

    return EXIT_SUCCESS;
}
