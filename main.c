#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include "pci_codes.h"

int main(int argc, char *argv[])
{
    if (iopl(3)) {
        perror("Don't have enough rights");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
