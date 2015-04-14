#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/io.h>

#include "pci_codes.h"


#define ALL_IO    3
#define RVAL_ERR -1
#define DFLT_DESCR "…"
#define NULL_DEV_REG_DATA  0x0FFFFFFFF

#define CONFIG_ADDRESS 0x0CF8
#define CONFIG_DATA    0x0CFC
#define PCI_MAX_BUS_COUNT ( 1 << 8 )
#define PCI_MAX_DEV_COUNT ( 1 << 5 )
#define PCI_MAX_FU_COUNT  ( 1 << 3 )

#define SET_BUS( bus ) ( bus << 16 )
#define SET_DEV( dev ) ( dev << 11 )
#define SET_FU( fu )   ( fu << 8 )

#define GET_REG_ADDR( bus, dev, fu, offs ) \
    ( (1 << 31) | SET_BUS(bus) | SET_DEV(dev) | SET_FU(fu) | offs )
#define GET_VENDOR_ID( dat )        ( (dat >> 0)  & 0xFFFF )
#define GET_DEV_ID( dat )           ( (dat >> 16) & 0xFFFF )
#define GET_CLASS_BASE( dat )       ( (dat >> (8 + 16)) & 0xFF )
#define GET_CLASS_SUB( dat )        ( (dat >> (8 + 8)) & 0xFF )
#define GET_CLASS_INTERFACE( dat )  ( (dat >> 8) & 0xFF )
#define GET_REVISION_ID( dat )      ( dat & 0xFF )
#define GET_SUBSYS_VENDOR_ID( dat ) ( dat & 0xFFFF )
#define GET_CLASS_CODE( cl )  \
            cl.base = GET_CLASS_BASE(regdat) ;       \
            cl.sub = GET_CLASS_SUB(regdat);          \
            cl.interf = GET_CLASS_INTERFACE(regdat);

typedef struct {
    uint8_t base;   /* base class */
    uint8_t sub;    /* subclass  */
    uint8_t interf; /* program interface */
} classCode_t;

typedef struct {
    const char * name;
    const char * desc;
} devInfo_t;

typedef struct {
    const char * base;
    const char * sub;
    const char * interf;
} classInfo_t;

int increasePrivelegies(uint8_t level);

const char * getVenName(uint16_t venId);
devInfo_t getDevInfo(uint16_t devId, uint16_t venId);
classInfo_t getClassInfo(classCode_t* cl);

int main(int argc, char *argv[])
{
    const char * venName;
    uint8_t  dev, maxDevsCount = PCI_MAX_DEV_COUNT, maxFuCount = PCI_MAX_FU_COUNT;
    uint16_t bus, maxBusCount  = PCI_MAX_BUS_COUNT;
    uint16_t venId, devId;
    uint8_t  rev, fu;
    uint16_t svenId;
    uint32_t regdat;

    classCode_t cl;
    devInfo_t devinf;
    classInfo_t clInf;

    if(increasePrivelegies(ALL_IO)) exit(EXIT_FAILURE);

    puts("bus,dev,f|vendor| dev | class bsi|rev|svenId| descriptions ...");
    puts("---------+------+-----+----------+---+------+-----------------");

    for(bus = 0; bus < maxBusCount; bus++) {
        for (dev = 0; dev < maxDevsCount; dev++) {
            for(fu = 0; fu < maxFuCount; fu++) {
                outl(GET_REG_ADDR(bus, dev, fu, 0x0), CONFIG_ADDRESS);
                regdat = inl(CONFIG_DATA);

                if(regdat == NULL_DEV_REG_DATA) continue;

                venId = GET_VENDOR_ID(regdat);
                devId = GET_DEV_ID(regdat);

                outl(GET_REG_ADDR(bus, dev, fu, 0x8), CONFIG_ADDRESS);
                regdat = inl(CONFIG_DATA);
                rev = GET_REVISION_ID(regdat);
                GET_CLASS_CODE(cl);

                outl(GET_REG_ADDR(bus, dev, fu, 0x2C), CONFIG_ADDRESS);
                svenId = GET_SUBSYS_VENDOR_ID(inl(CONFIG_DATA));
                venName = getVenName(venId);
                devinf = getDevInfo(devId, venId);
                clInf = getClassInfo(&cl);

                printf("%3x,%2x,%x | %04x | %04x| %02x,%02x,%02x | %2x| %4x | ",
                         bus,dev,fu, venId, devId, cl.base,cl.sub,cl.interf, rev, svenId);
                printf("%s: %s %s %s %s %s",
                        venName, devinf.name, devinf.desc, clInf.base, clInf.sub, clInf.interf);
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

const char * getVenName(uint16_t venId)
{
    static int i;
    for (i = 0; i < PCI_VENTABLE_LEN; ++i) {
        if(venId == PciVenTable[i].VenId) return PciVenTable[i].VenFull;
    }

    return DFLT_DESCR;
}

devInfo_t getDevInfo(uint16_t devId, uint16_t venId)
{
    static int i;
    static devInfo_t devinf = { .name = DFLT_DESCR, .desc = DFLT_DESCR };
    for (i = 0; i < PCI_DEVTABLE_LEN; i++) {
        if (devId == PciDevTable[i].DevId && venId == PciDevTable[i].VenId) {
            devinf.name = PciDevTable[i].Chip;
            devinf.desc = PciDevTable[i].ChipDesc;
            return devinf;
        }
    }
    return devinf;
}

classInfo_t getClassInfo(classCode_t* cl)
{
    static int i;
    static classInfo_t clInf = {
        .base = DFLT_DESCR, .sub = DFLT_DESCR, .interf = DFLT_DESCR
    };
    for (i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (cl->base   == PciClassCodeTable[i].BaseClass &&
            cl->sub    == PciClassCodeTable[i].SubClass &&
            cl->interf == PciClassCodeTable[i].ProgIf
            ) {
            clInf.base   = PciClassCodeTable[i].BaseDesc;
            clInf.sub    = PciClassCodeTable[i].SubDesc;
            clInf.interf = PciClassCodeTable[i].ProgDesc;
            return clInf;
        }
    }

    return clInf;
}

