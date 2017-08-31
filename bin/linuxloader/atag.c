/* example.c
 * example ARM Linux bootloader code
 * this example is distributed under the BSD licence
 */

#include "atag.h"
#include <stdint.h>

#define tag_next(t)     ((struct tag *)((uint32_t *)(t) + (t)->hdr.size))
#define tag_size(type)  ((sizeof(struct atag_header) + sizeof(struct type)) >> 2)
static struct atag *params; /* used to point at the current tag */

void
setup_core_tag(void * address,long pagesize)
{
    params = (struct tag *)address;         /* Initialise parameters to start at given address */

    params->hdr.tag = ATAG_CORE;            /* start with the core tag */
    params->hdr.size = tag_size(atag_core); /* size the tag */

    params->u.core.flags = 1;               /* ensure read-only */
    params->u.core.pagesize = pagesize;     /* systems pagesize (4k) */
    params->u.core.rootdev = 0;             /* zero root device (typicaly overidden from commandline )*/

    params = tag_next(params);              /* move pointer to next tag */
}

void
setup_ramdisk_tag(uint32_t size)
{
    params->hdr.tag = ATAG_RAMDISK;         /* Ramdisk tag */
    params->hdr.size = tag_size(atag_ramdisk);  /* size tag */

    params->u.ramdisk.flags = 0;            /* Load the ramdisk */
    params->u.ramdisk.size = size;          /* Decompressed ramdisk size */
    params->u.ramdisk.start = 0;            /* Unused */

    params = tag_next(params);              /* move pointer to next tag */
}

void
setup_initrd2_tag(uint32_t start, uint32_t size)
{
    params->hdr.tag = ATAG_INITRD2;         /* Initrd2 tag */
    params->hdr.size = tag_size(atag_initrd2);  /* size tag */

    params->u.initrd2.start = start;        /* physical start */
    params->u.initrd2.size = size;          /* compressed ramdisk size */

    params = tag_next(params);              /* move pointer to next tag */
}

void
setup_mem_tag(uint32_t start, uint32_t len)
{
    params->hdr.tag = ATAG_MEM;             /* Memory tag */
    params->hdr.size = tag_size(atag_mem);  /* size tag */

    params->u.mem.start = start;            /* Start of memory area (physical address) */
    params->u.mem.size = len;               /* Length of area */

    params = tag_next(params);              /* move pointer to next tag */
}

void
setup_cmdline_tag(const char * line)
{
    int linelen = strlen(line);

    if(!linelen)
        return;                             /* do not insert a tag for an empty commandline */

    params->hdr.tag = ATAG_CMDLINE;         /* Commandline tag */
    params->hdr.size = (sizeof(struct atag_header) + linelen + 1 + 4) >> 2;

    strcpy(params->u.cmdline.cmdline,line); /* place commandline into tag */

    params = tag_next(params);              /* move pointer to next tag */
}

void
setup_end_tag(void)
{
    params->hdr.tag = ATAG_NONE;            /* Empty tag ends list */
    params->hdr.size = 0;                   /* zero length */
}
extern uint32_t _binary_bin_linuxloader_initramfs_start;
extern uint32_t _binary_bin_linuxloader_initramfs_end;
void
setup_tags(void)
//setup_tags(void *parameters)
{
    uint32_t initrd_start = (uint32_t) &_binary_bin_linuxloader_initramfs_start;
    uint32_t initrd_end = (uint32_t) &_binary_bin_linuxloader_initramfs_end;
    uint32_t initrd_size = initrd_end-initrd_start;
    setup_core_tag(0x30000100, 4096);       /* standard core tag 4k pagesize */
    setup_mem_tag(0x30000000, 0x2000000);    /* 64Mb at 0x10000000 */
    //setup_ramdisk_tag(4096);                /* create 4Mb ramdisk */
    //setup_initrd2_tag(INITRD_LOAD_ADDRESS, 0x100000); /* 1Mb of compressed data placed 8Mb into memory */
    setup_cmdline_tag("init=/sbin/init root=/dev/ram console=ttySAC0,115200 earlyprintk");    /* commandline setting root device */
    //setup_cmdline_tag("init=/sbin/init root=/dev/ram console=tty0 earlyprintk");    /* commandline setting root device */
    //setup_ramdisk_tag(1024);
    setup_initrd2_tag(initrd_start,initrd_size);
    setup_end_tag();                    /* end of tags */
}

