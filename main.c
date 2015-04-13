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

#define GET_REG_ADDR( bus, dev, offs ) ( (1 << 31) | SET_BUS(bus) | SET_DEV(dev) | offs )

#define GET_VENDOR_ID( dat )       ( (dat >> 0)  & 0xFFFF )
#define GET_DEV_ID( dat )          ( (dat >> 16) & 0xFFFF )

#define GET_CLASS_BASE( dat )      ( (dat >> (8 + 16)) & 0xFF )
#define GET_CLASS_SUB( dat )       ( (dat >> (8 + 8)) & 0xFF )
#define GET_CLASS_INTERFACE( dat ) ( (dat >> 8) & 0xFF )
#define GET_REVISION_ID( dat )     ( dat & 0xFF )
#define GET_SUBSYS_VENDOR_ID( dat )  ( dat & 0xFFFF )

typedef struct {
    uint8_t base;  /* base class */
    uint8_t sub;   /* subclass  */
    uint8_t interf; /* program interface */
} classCode_t;

int increasePrivelegies(uint8_t level);

int main(int argc, char *argv[])
{
    uint8_t  dev, maxDevsCount = PCI_MAX_DEV_COUNT;
    uint16_t bus, maxBusCount  = PCI_MAX_BUS_COUNT;
    uint16_t vendorId, devId;
    uint8_t  rev;
    uint32_t regdat;
    uint16_t svendor;
    classCode_t cl;

    if(increasePrivelegies(ALL_IO)) exit(EXIT_FAILURE);

    puts(" bus,dev | vendor | dev id | class b,s,i | rev  | svendor");
    puts("---------+--------+--------+-------------+------+---------");

    for(bus = 0; bus < maxBusCount; bus++) {
        for (dev = 0; dev < maxDevsCount; dev++) {
            outl(GET_REG_ADDR(bus, dev, 0x0), CONFIG_ADDRESS);
            regdat = inl(CONFIG_DATA);

            if(regdat != NULL_DEV_REG_DATA) {
                vendorId = GET_VENDOR_ID(regdat);
                devId = GET_DEV_ID(regdat);

                outl(GET_REG_ADDR(bus, dev, 0x8), CONFIG_ADDRESS);
                regdat = inl(CONFIG_DATA);
                rev = GET_REVISION_ID(regdat);
                cl.base = GET_CLASS_BASE(regdat);
                cl.sub = GET_CLASS_SUB(regdat);
                cl.interf = GET_CLASS_INTERFACE(regdat);

                outl(GET_REG_ADDR(bus, dev, 0x2C), CONFIG_ADDRESS);

                svendor = GET_SUBSYS_VENDOR_ID(inl(CONFIG_DATA));

                printf(" %3x,%-2x  |  %04x  |  %04x  |   %02x,%02x,%02x  |  %2x  |  %4x  ",
                         bus,dev, vendorId, devId,  cl.base,cl.sub,cl.interf, rev, svendor);
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

