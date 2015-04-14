#include <stdio.h>
#include <stdlib.h>

#if _WIN32
# error Application does not support Windows yet
#else
# include <sys/io.h>
# include <sys/stat.h>
#endif
#include "pci_codes.h"

#ifdef _MSC_VER  // For the Visual Studio (with the expectation of cross-platform feature in the future)
#  define INLINE static __forceinline
#else
#  ifdef __GNUC__
#    define INLINE static inline __attribute__((always_inline))
#  else
#    define INLINE static inline
#  endif
#endif

#if __STDC_VERSION__ >= 199901L
# include <stdint.h>
  typedef uint8_t        U8;
  typedef uint16_t       U16;
  typedef uint32_t       U32;
#else
  typedef unsigned char  U8;
  typedef unsigned short U16;
  typedef unsigned int   U32;
#endif

#define ALL_IO    3
#define RVAL_ERR -1
#define DFLT_DESCR "…"

#define NULL_DEV           0x0FFFFFFFF
#define CONFIG_ADDRESS     0x0CF8
#define CONFIG_DATA        0x0CFC
#define PCI_MAX_BUS_COUNT  ( 1 << 8 )
#define PCI_MAX_DEV_COUNT  ( 1 << 5 )
#define PCI_MAX_FU_COUNT   ( 1 << 3 )

#define SET_BUS( bus )     ( bus << 16 )
#define SET_DEV( dev )     ( dev << 11 )
#define SET_FU( fu )       ( fu << 8 )

#define GET_VENDOR_ID( dat )        ( (dat >> 0)        & 0xFFFF )
#define GET_DEV_ID( dat )           ( (dat >> 16)       & 0xFFFF )
#define GET_CLASS_BASE( dat )       ( (dat >> (8 + 16)) & 0xFF )
#define GET_CLASS_SUB( dat )        ( (dat >> (8 + 8))  & 0xFF )
#define GET_CLASS_INTERFACE( dat )  ( (dat >> 8)        & 0xFF )
#define GET_REVISION_ID( dat )      (  dat & 0xFF )
#define GET_SUBSYS_VENDOR_ID( dat ) (  dat & 0xFFFF )
#define GET_REG_ADDR( bus, dev, fu, offs ) \
    ( (1 << 31) | SET_BUS(bus) | SET_DEV(dev) | SET_FU(fu) | offs )

typedef struct {
    U8 base;   /* base class */
    U8 sub;    /* subclass  */
    U8 interf; /* program interface */
} classId_t;

typedef struct {
    const char * name;
    const char * desc;
} devInfo_t;

typedef struct {
    const char * base;
    const char * sub;
    const char * interf;
} classInfo_t;

U8 increasePrivelegies(U8 level);
INLINE void findVenName(U16 venId, const char ** venName);
INLINE void findDevInfo(U16 devId, U16 venId, devInfo_t* devinf);
INLINE void findClassInfo(classId_t* cl, classInfo_t* clInf);
INLINE U32 getRegData(U16 bus, U8 dev, U8 fu, U8 offs);

int main(int argc, char *argv[])
{
    const char * venName;
    U8  dev, rev, fu, maxDevsCount = PCI_MAX_DEV_COUNT, maxFuCount = PCI_MAX_FU_COUNT;
    U16 bus, svenId, venId, devId, maxBusCount  = PCI_MAX_BUS_COUNT;
    U32 regdat;
    classId_t cl;
    devInfo_t devinf;
    classInfo_t clInf;

    if(increasePrivelegies(ALL_IO) == RVAL_ERR) exit(EXIT_FAILURE);

    puts("bus,dev,f|vendor| dev | class bsi|rev|svenId| descriptions ...");
    puts("---------+------+-----+----------+---+------+-----------------");

    for(bus = 0; bus < maxBusCount; bus++) {
        for (dev = 0; dev < maxDevsCount; dev++) {
            for(fu = 0; fu < maxFuCount; fu++) {
                if((regdat = getRegData(bus, dev, fu, 0x0)) == NULL_DEV)
                    continue;

                venId = GET_VENDOR_ID(regdat);
                devId = GET_DEV_ID(regdat);

                regdat    = getRegData(bus, dev, fu, 0x8);
                rev       = GET_REVISION_ID(regdat);
                cl.base   = GET_CLASS_BASE(regdat) ;
                cl.sub    = GET_CLASS_SUB(regdat);
                cl.interf = GET_CLASS_INTERFACE(regdat);

                regdat = getRegData(bus, dev, fu, 0x2C);
                svenId = GET_SUBSYS_VENDOR_ID(regdat);

                findVenName(venId, &venName);
                findDevInfo(devId, venId, &devinf);
                findClassInfo(&cl, &clInf);

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

U8 increasePrivelegies(U8 level)
{
    if (iopl(level)) {
        perror("ERROR iopl");
        return RVAL_ERR;
    }

    return EXIT_SUCCESS;
}

INLINE U32 getRegData(U16 bus, U8 dev, U8 fu, U8 offs)
{
    outl(GET_REG_ADDR(bus, dev, fu, offs), CONFIG_ADDRESS);
    return inl(CONFIG_DATA);
}

INLINE void findVenName(U16 venId, const char ** venName)
{
    int i;
    for (i = 0; i < PCI_VENTABLE_LEN; ++i) {
        if(venId == PciVenTable[i].VenId) {
            *venName = PciVenTable[i].VenFull;
            return ;
        }
    }
    *venName = DFLT_DESCR;
}

INLINE void findDevInfo(U16 devId, U16 venId, devInfo_t* devinf)
{
    int i;
    for (i = 0; i < PCI_DEVTABLE_LEN; i++) {
        if (devId == PciDevTable[i].DevId && venId == PciDevTable[i].VenId) {
            devinf->name = PciDevTable[i].Chip;
            devinf->desc = PciDevTable[i].ChipDesc;
            return ;
        }
    }
    *devinf = (devInfo_t){ .name = DFLT_DESCR, .desc = DFLT_DESCR };
}

INLINE void findClassInfo(classId_t* cl, classInfo_t* clInf)
{
    static int i;
    for (i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (cl->base   == PciClassCodeTable[i].BaseClass &&
            cl->sub    == PciClassCodeTable[i].SubClass &&
            cl->interf == PciClassCodeTable[i].ProgIf
            ) {
            clInf->base   = PciClassCodeTable[i].BaseDesc;
            clInf->sub    = PciClassCodeTable[i].SubDesc;
            clInf->interf = PciClassCodeTable[i].ProgDesc;
            return ;
        }
    }
    *clInf = (classInfo_t) {
        .base = DFLT_DESCR, .sub = DFLT_DESCR, .interf = DFLT_DESCR
    };
}

