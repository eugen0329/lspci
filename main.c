#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/io.h>

#include "pci_codes.h"

#define ALL_IO    3
#define RVAL_ERR -1
#define CONFIG_ADDRESS 0x0CF8
#define CONFIG_DATA    0x0CFC

#define NULL_DEV_REG_DATA  0x0FFFFFFFF
#define PCI_MAX_BUS_COUNT ( 1 << 8 )
#define PCI_MAX_DEV_COUNT ( 1 << 5 )

#define SET_BUS( bus ) ( bus << 16 )
#define SET_DEV( dev ) ( dev << 11 )

#define GET_REG_ADDR( bus, dev ) ( (1 << 31) | SET_BUS(bus) | SET_DEV(dev) )
#define GET_VENDOR_ID( dat ) ( (dat >> 0)  & 0xFFFF )
#define GET_DEV_ID( dat )    ( (dat >> 16) & 0xFFFF )


int increasePrivelegies(uint8_t level);

int main(int argc, char *argv[])
{
    uint8_t  dev, maxDevsCount = PCI_MAX_DEV_COUNT;
    uint16_t bus, maxBusCount  = PCI_MAX_BUS_COUNT;
    uint16_t vendorId, devId;
    uint32_t regdat;

    if(increasePrivelegies(ALL_IO)) exit(EXIT_FAILURE);

    puts(" bus, dev | vendor id | dev id  ");
    puts("----------+-----------+---------");

    for(bus = 0; bus < maxBusCount; bus++) {
        for (dev = 0; dev < maxDevsCount; dev++) {
            int regAddr = GET_REG_ADDR(dev, bus );

            outl(regAddr, CONFIG_ADDRESS);
            regdat = inl(CONFIG_DATA);

            if(regdat != NULL_DEV_REG_DATA) {
                vendorId = GET_VENDOR_ID(regdat);
                devId = GET_DEV_ID(regdat);
                printf(" %3x, %-2x  |    %4x   |  %4x   ", bus, dev, vendorId, devId);
                puts("");
            }
        }
    }

    return EXIT_SUCCESS;
}

int increasePrivelegies(uint8_t level)
{
    if (iopl(level)) {
        perror("ERROR iopl");
        return RVAL_ERR;
    }
    return 0;
}

