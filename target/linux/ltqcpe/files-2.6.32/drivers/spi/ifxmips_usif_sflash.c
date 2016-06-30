/******************************************************************************
**
** FILE NAME    : ifxmips_usif_sflash.c
** PROJECT      : IFX UEIP
** MODULES      : 25 types of Serial Flash 
**
** DATE         : 16 Oct 2009
** AUTHOR       : Lei Chuanhua
** DESCRIPTION  : SPI Flash MTD Driver
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 16,Oct 2009 Lei Chuanhua    Initial UEIP release
*******************************************************************************/
/*!
  \defgroup IFX_USIF_SFLASH IFX USIF SPI flash module
  \brief ifx usif spi serial flash driver module
*/

/*!
  \defgroup IFX_USIF_SFLASH_OS OS APIs
  \ingroup IFX_USIF_SFLASH
  \brief IFX serial flash driver OS interface functions
*/

/*!
  \defgroup IFX_USIF_SFLASH_INTERNAL Internal functions
  \ingroup IFX_USIF_SFLASH
  \brief IFX serial flash driver functions
*/

/*!
  \file ifxmips_usif_sflash.c
  \ingroup IFX_USIF_SFLASH
  \brief ifx serial flash driver file
*/
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mtd/mtd.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#include <../drivers/mtd/mtdcore.h>
#endif
#include <linux/mtd/partitions.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#include <linux/math64.h>
#endif 
/* Project header */
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_gpio.h>
#include "ifxmips_usif_sflash.h"

#define IFX_USIF_SFLASH_VER_MAJOR          1
#define IFX_USIF_SFLASH_VER_MID            0
#define IFX_USIF_SFLASH_VER_MINOR          8

#define IFX_USIF_SFLASH_NAME               "ifx_usif_flash"

#define IFX_SFLASH_ADDR_CYCLES              3   /* 24bit address */

//#define IFX_SPI_FLASH_DBG

#if defined(IFX_SPI_FLASH_DBG)
#define IFX_SFLASH_PRINT(format, arg...)   \
    printk("%s: " format, __func__, ##arg)
#define INLINE 
#else
#define IFX_SFLASH_PRINT(format, arg...)   \
    do {} while(0)
#define INLINE inline
#endif

extern const struct mtd_partition g_ifx_mtd_spi_partitions[IFX_SPI_FLASH_MAX][IFX_MTD_SPI_PART_NB];

static struct semaphore ifx_sflash_sem;

static ifx_usif_spi_dev_t *spi_sflash = NULL;

/* 
 * NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static struct ifx_usif_sflash_manufacturer_info __devinitdata flash_manufacturers[] = {
    {   
        /* Spansion -- single (large) sector size only, at least
         * for the chips listed here (without boot sectors).
         */
        .name = "Spansion",
        .id = JED_MANU_SPANSION,
        .flashes = {
            { "S25Sl004", 0x0212, 64 * 1024, 8, },
            { "S25Sl008", 0x0213, 64 * 1024, 16, },
            { "S25LF016", 0x0214, 64 * 1024, 32, },
            { "S25LF032", 0x0215, 64 * 1024, 64, },
            { "S25LF064", 0x0216, 64 * 1024, 128, },
            { "", 0x0, 0, 0 },
            { "S25LF0128", 0x0218, 256 * 1024, 64, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
        },
    },   
    {   /* ST Microelectronics -- newer production may have feature updates */
        .name = "ST",
        .id = JED_MANU_ST,
        .flashes = {
            { "m25p05", 0x2010, 32 * 1024, 2, },
            { "m25p10", 0x2011, 32 * 1024, 4, },
            { "m25p20", 0x2012, 64 * 1024, 4, },
            { "m25p40", 0x2013, 64 * 1024, 8, },
            { "m25p16", 0x2015, 64 * 1024, 32, },
            { "m25px16", 0x7115, 64 * 1024, 32, SECT_4K, },            
            { "m25p32", 0x2016, 64 * 1024, 64, },
            { "m25px32",0x7116, 64 * 1024, 64, SECT_4K, },
            { "m25p64", 0x2017, 64 * 1024, 128, },
            { "m25px64",0x7117, 64 * 1024, 128, SECT_4K, },
            { "m25p128", 0x2018, 256 * 1024, 64, },
            { "m45pe80", 0x4014,  64 * 1024, 16, },
            { "m45pe16", 0x4015,  64 * 1024, 32, },
            { "m25pe80", 0x8014,  64 * 1024, 16, },
            { "m25pe16", 0x8015,  64 * 1024, 32, SECT_4K, },
           
        },
    },
    {   /* SST -- large erase sizes are "overlays", "sectors" are 4K */
        .name = "SST",
        .id = JED_MANU_SST,
        .flashes = {
            { "sst25vf040b", 0x258d, 64 * 1024, 8, SECT_4K, },
            { "sst25vf080b", 0x258e, 64 * 1024, 16, SECT_4K, },
            { "sst25vf016b", 0x2541, 64 * 1024, 32, SECT_4K, },
            { "sst25vf032b", 0x254a, 64 * 1024, 64, SECT_4K, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
        },
    },   
    {
        .name = "Atmel",
        .id = JED_MANU_ATMEL,
        .flashes = {
            { "at25fs010",  0x6601, 32 * 1024, 4, SECT_4K, },
            { "at25fs040",  0x6604, 64 * 1024, 8, SECT_4K, },
            { "at25df041a", 0x4401, 64 * 1024, 8, SECT_4K, },
            { "at25df641",  0x4800, 64 * 1024, 128, SECT_4K, },
            { "at26f004",   0x0400, 64 * 1024, 8, SECT_4K, },
            { "at26df081a", 0x4501, 64 * 1024, 16, SECT_4K, },
            { "at26df161a", 0x4601, 64 * 1024, 32, SECT_4K, },
            { "at26df321",  0x4701, 64 * 1024, 64, SECT_4K, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
        },
       
    },
    {    /* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
        .name = "Winbond",
        .id = JED_MANU_WINBOND,
        .flashes = {
            { "w25x10", 0x3011, 64 * 1024, 2, SECT_4K, },
            { "w25x20", 0x3012, 64 * 1024, 4, SECT_4K, },
            { "w25x40", 0x3013, 64 * 1024, 8, SECT_4K, },
            { "w25x80", 0x3014, 64 * 1024, 16, SECT_4K, },
            { "w25x16", 0x3015, 64 * 1024, 32, SECT_4K, },
            { "w25x32", 0x3016, 64 * 1024, 64, SECT_4K, },
            { "w25x64", 0x3017, 64 * 1024, 128, SECT_4K, },
            { "W25P80", 0x2014, 256 * 256, 16, },
            { "W25P16", 0x2015, 256 * 256, 32, },
            { "W25P32", 0x2016, 256 * 256, 64, },
            { "W25P64", 0x2017, 256 * 256, 128, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
        },
        
    },
    {
        .name = "MX",
        .id = JED_MANU_MX,
        .flashes = {
            { "MX25P2005", 0x2012, 64 * 1024, 4, SECT_4K },
            { "MX25P4005", 0x2013, 64 * 1024, 8, SECT_4K },
            { "MX25P8005", 0x2014, 64 * 1024, 16, },
            { "MX25P1605", 0x2015, 64 * 1024, 32, SECT_4K },
            { "MX25P3205", 0x2016, 64 * 1024, 64, },
            { "MX25P6405", 0x2017, 64 * 1024, 128, },
            { "MX25L12835", 0x2018, 64 * 1024, 256, SECT_4K },
            { "MX25P12855", 0x2618, 64 * 1024, 256, },
            { "MX25L25635", 0x2019, 64 * 1024, 512, },
            { "MX25P25655", 0x2619, 64 * 1024, 512, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
            { "", 0, 0, 0, },
        },
    },
};

#ifdef CONFIG_MTD_PARTITIONS
#ifdef CONFIG_MTD_CMDLINE_PARTS
static const char *part_probes[] = { "cmdlinepart", NULL };
#endif 
#endif /* CONFIG_MTD_PARTITIONS */

#ifdef IFX_SPI_FLASH_DBG
static void 
flash_dump(const char *title, const u8 *buf, size_t len)
{
    int i, llen, lenlab = 0;
    const u8 *pos = buf;
    const int line_len = 16;

    printk("%s - hex_ascii(len=%lu):\n", title, (unsigned long) len);
    while (len) {
        llen = len > line_len ? line_len : len;
        printk("%08x: ", lenlab);
        for (i = 0; i < llen; i++)
            printk(" %02x", pos[i]);
        for (i = llen; i < line_len; i++)
            printk("   ");
        printk("   ");
        for (i = 0; i < llen; i++) {
            if (isprint(pos[i]))
                printk("%c", pos[i]);
            else
                printk(".");
        }
        for (i = llen; i < line_len; i++)
            printk(" ");
        printk("\n");
        pos += llen;
        len -= llen;
        lenlab += line_len;
    }
}
#endif /* IFX_SPI_FLASH_DBG */

/**  
 * \fn static INLINE void  u32_to_u8_addr(u32 address, u8* addr)
 * \brief  convert address from u32 to u8 array 
 *  
 * \param[in]  address the address to be converted, maximum 32 bit
 * \param[out] addr array that holds the results, maximum 4 elemets
 * \ingroup IFX_USIF_SFLASH_INTERNAL
 */
static INLINE void 
u32_to_u8_addr(u32 address, u8* addr)
{
    addr[0] = (u8) ((address >> (spi_sflash->addr_cycles * 8 - 8)) & 0xff);
    addr[1] = (u8) ((address >> (spi_sflash->addr_cycles * 8 - 16)) & 0xff);
    addr[2] = (u8) ((address >> (spi_sflash->addr_cycles * 8 - 24)) & 0xff);
    addr[3] = (u8) ((address >> (spi_sflash->addr_cycles * 8 - 32)) & 0xff);

}

/**
 * \fn static int ifx_usif_sflash_rdsr(ifx_usif_spi_dev_t *dev, u8 *status)
 * \brief Return the status of the serial flash device.
 *
 * \param[in]  dev   Pointer to ifx_usif_spi_dev_t
 * \param[out] status Pointer to return status
 * \return   -l  Failed to read status
 * \return    0  OK
 * \ingroup IFX_USIF_SFLASH_INTERNAL
 */
static int 
ifx_usif_sflash_rdsr(ifx_usif_spi_dev_t *dev, u8 *status)
{
    int ret;
    u8  cmd = IFX_OPCODE_RDSR;
    
    if ((ret = ifx_usif_spiTxRx(dev->sflash_handler ,(char *)&cmd, sizeof(u8), status, sizeof(u8))) != 2){
        IFX_SFLASH_PRINT("line %d ifx_usif_spiTxRx fails %d\n", __LINE__, ret);
        return -1;
    }
    return 0;
}

/**
 * \fn static int ifx_usif_sflash_sync(ifx_usif_spi_dev_t *dev) 
 * \brief Poll the serial flash device until it is READY
 *
 * \param[in]  dev   Pointer to ifx_usif_spi_dev_t
 * \return     -l  Failed to read status
 * \return     0  OK
 * \ingroup IFX_USIF_SFLASH_INTERNAL
 */
static int 
ifx_usif_sflash_sync(ifx_usif_spi_dev_t *dev) 
{
    int ret = 0;
    u8 status;
    int count = 0;
    unsigned long deadline;

    deadline = jiffies + IFX_MAX_READY_WAIT_JIFFIES;
    do {
       ret = ifx_usif_sflash_rdsr(dev, &status);
        if (ret < 0) {
            IFX_SFLASH_PRINT("Read back status fails %d\n", ret);
            break;
        }
 
        if (!(status & IFX_SR_WIP)) {
            return 0;
        }
        cond_resched();

        /* This is mainly for detecting serial flash */
        if (++count > IFX_SFLASH_DETECT_COUNTER) {
            IFX_SFLASH_PRINT("Detct counter out of range!!!\n");
            break;
        }        

    } while (!time_after_eq(jiffies, deadline));

    return -1;
}

/**
 * \fn static int ifx_usif_sflash_session(ifx_usif_spi_dev_t *dev, u8 cmd, u8 *addr, u8 dummy_cycles, 
 *                u8 * wbuf, u32 wcnt, u8 * rbuf, u32 rcnt)
 * \brief Handle serial flash read/write/erase in one central function
 *
 * \param[in]  dev   Pointer to ifx_usif_spi_dev_t
 * \param[in]  cmd   Serial flash command code
 * \param[in]  addr  Flash address offset
 * \param[in]  dummy_cycles Dummy cycles for some serial flash
 * \param[in]  wbuf     Pointer to the data packet to write.
 * \param[in]  wcnt     Amount of Bytes to write.
 * \param[out] rbuf     Pointer to store the read data.
 * \param[in]  rcnt     Amount of Bytes to read.
 * \return     -EINVAL  Invalid read data length
 * \return     -EBUSY   Serial flash device is busy
 * \return     0        OK
 * \ingroup IFX_USIF_SFLASH_INTERNAL
 */
static int 
ifx_usif_sflash_session(ifx_usif_spi_dev_t *dev, u8 cmd, u8 *addr, u8 dummy_cycles, 
                 u8 *wbuf, u32 wcnt, u8 *rbuf, u32 rcnt)
{
    int i;
    int ret = 0;
    int err = 0;
    int start = 0;
    int total = 0;
    char *buf = dev->flash_tx_buf;  /* Always use the same buffer for efficiecy */
    char *tbuf;

    /* Sanity check */
    if (unlikely(rcnt >= IFX_SFLASH_MAX_READ_SIZE)) {
        printk("%s: please increase read buffer size\n", __func__);
        return -EINVAL;
    }
    
    /* CMD */
    buf[0] = cmd;
    start = 1;

    /* Address */
    if (addr != NULL) {
        for (i = 0; i < dev->addr_cycles; i++) {
            buf[start + i] = addr[i];
        }
        start += dev->addr_cycles;
    }

    /* Dummy cycles */
    if (dummy_cycles > 0 && dummy_cycles < IFX_MAX_DUMMY_CYCLES) {
        for (i = 0; i < dummy_cycles; i ++) {
            buf[start + i] = 0;
        }
        start += dummy_cycles;
    }   
    
    /* Possibly, there is no flash mounted */
    if (ifx_usif_sflash_sync(dev) == -1)
        return -EBUSY;
    
    if ((wcnt == 0) && (rcnt == 0)) { /* Cmd + Addr + Dummy cycles */
        if ((ret = ifx_usif_spiTx(dev->sflash_handler, buf, start)) != start) {
            err++;
            IFX_SFLASH_PRINT("line %d ifx_usif_spiTx fails %d\n", __LINE__, ret);
            goto sflash_session_out;
        }
    }
    else if (wcnt > 0) { /* Cmd + Addr +  Dummy cycles + Write data */
        total = start + wcnt;
        memcpy(buf + start, wbuf, wcnt);
        if ((ret = ifx_usif_spiTx(dev->sflash_handler, buf, total)) != total) {
            err++;
            IFX_SFLASH_PRINT("line %d ifx_usif_spiTx fails %d\n", __LINE__, ret);
            goto sflash_session_out;
        }
    }
    else if (rcnt > 0) { /* Cmd + Addr +  Dummy cycles + Read data */
        int rx_aligned = 0;
        
        total = start + rcnt;
        rx_aligned = (((u32)rbuf)& (IFX_SFLASH_DMA_MAX_BURST_LEN - 1)) == 0 ? 1 : 0;
        if (rx_aligned == 0){
            tbuf = dev->flash_rx_buf;
        }
        else {
            tbuf = rbuf;
        }
        if ((ret = ifx_usif_spiTxRx(dev->sflash_handler, buf, start, tbuf, rcnt)) != total) {
            err++;
            IFX_SFLASH_PRINT("line %d ifx_usif_spiTxRx fails %d\n", __LINE__, ret);
            goto sflash_session_out;
        }
        
        if (rx_aligned == 0) {
            memcpy(rbuf, tbuf, rcnt);
        }
        else {
            /* Do nothing */
        }
    }
    else {
        printk("%s should never happen\n", __func__);
    }
sflash_session_out: 
    return err; 
}

static INLINE int 
ifx_sflash_wren(void)
{ 
    u8 cmd = IFX_OPCODE_WREN;
    return ifx_usif_sflash_session(spi_sflash, cmd, NULL, 0, NULL, 0, NULL, 0);
}

static INLINE int
ifx_sflash_se(u8 *addr)
{
    return ifx_usif_sflash_session(spi_sflash, spi_sflash->erase_opcode, addr, 0, NULL, 0, NULL, 0);
}

static INLINE int 
ifx_sflash_pp(u8 *addr, u8 *buf, u32 len)
{
    u8 cmd = IFX_OPCODE_PP;
    return ifx_usif_sflash_session(spi_sflash, cmd, addr, 0, buf, len, NULL, 0);
}

static INLINE int 
ifx_sflash_rd(u8 *addr, u8 *buf, u32 len)
{
    u8 cmd = IFX_OPCODE_READ;
    return ifx_usif_sflash_session(spi_sflash, cmd, addr, spi_sflash->dummy_cycles, NULL, 0, buf, len);   
}

static INLINE int 
ifx_spi_read(u32 saddr, u8 *buf, u32 len)
{
    int ret;
    u8 addr[IFX_MAX_ADDRESS_NUM] = {0};

    u32_to_u8_addr(saddr, addr);    
    ret = ifx_sflash_rd(addr, buf, len);
    return ret;
}

static INLINE int 
ifx_spi_set_4byte(ifx_usif_spi_dev_t *dev, int enable)
{
    u8 cmd;
    u8 data = 0;

    switch (dev->manufacturer_id) {
        case JED_MANU_MX:
            cmd = enable ? OPCODE_EN4B : OPCODE_EX4B;
            return ifx_usif_sflash_session(dev, cmd, NULL, 0, NULL, 0, NULL, 0);
        default:
            /* Spansion style */
            cmd = OPCODE_BRWR;
            data = enable << 7;
            return ifx_usif_sflash_session(dev, cmd, NULL, 0, &data, 1, NULL, 0);
        }
}

static INLINE int 
ifx_spi_write(u32 saddr, u8 *buf,u32 len)
{
    int ret;
    u8 addr[IFX_MAX_ADDRESS_NUM] = {0};

    u32_to_u8_addr(saddr, addr);
    ifx_sflash_wren();
    ret = ifx_sflash_pp(addr, buf, len);
    return ret;
}

static INLINE int 
ifx_spi_sector_erase(u32 saddr)
{
    u8 addr[IFX_MAX_ADDRESS_NUM] = {0};

    u32_to_u8_addr(saddr, addr);    
    ifx_sflash_wren();
    return ifx_sflash_se(addr);
}

static INLINE int
ifx_spi_chip_erase(void)
{
    u8 cmd = IFX_OPCODE_CHIP_ERASE;
    
    ifx_sflash_wren();
    return ifx_usif_sflash_session(spi_sflash, cmd, NULL, 0, NULL, 0, NULL, 0);   
}

/**
 * \fn static int ifx_usif_flash_probe(ifx_usif_spi_dev_t *pdev)
 * \brief Detect serial flash device
 *
 * \param[in]  pdev   Pointer to ifx_usif_spi_dev_t
 * \return   -l  Failed to detect device
 * \return    0  OK
 * \ingroup IFX_USIF_SFLASH_INTERNAL
 */
static int
ifx_usif_flash_probe(ifx_usif_spi_dev_t *pdev)
{
    int i;
    u16 dev_id;
    u8  cmd = IFX_OPCODE_RDID;

    ifx_usif_spiLock(pdev->sflash_handler);
    /* Send the request for the part identification */
    ifx_usif_spiTx(pdev->sflash_handler, &cmd, sizeof (cmd));
    /* Now read in the manufacturer id bytes */
    do {
        ifx_usif_spiRx(pdev->sflash_handler, &pdev->manufacturer_id, 1);
        if (pdev->manufacturer_id == 0x7F) {
            printk("Warning: unhandled manufacturer continuation byte!\n");
        }
    } while (pdev->manufacturer_id == 0x7F); 

    /* Now read in the first device id byte */
    ifx_usif_spiRx(pdev->sflash_handler, &pdev->device_id1, 1);
    /* Now read in the second device id byte */
    ifx_usif_spiRx(pdev->sflash_handler, &pdev->device_id2, 1);
    ifx_usif_spiUnlock(pdev->sflash_handler);
    dev_id = (pdev->device_id1 << 8) | pdev->device_id2;   
    IFX_SFLASH_PRINT("Vendor %02x Type %02x sig %02x\n", pdev->manufacturer_id, 
        pdev->device_id1, pdev->device_id2);
    for (i = 0; i < ARRAY_SIZE(flash_manufacturers); ++i) {
        if (pdev->manufacturer_id == flash_manufacturers[i].id) {
            break;
        }
    }
    if (i == ARRAY_SIZE(flash_manufacturers)){
        goto unknown;
    }
    pdev->manufacturer = &flash_manufacturers[i];
    for (i = 0; pdev->manufacturer->flashes[i].id; ++i) {
        if (dev_id == pdev->manufacturer->flashes[i].id) {
            break;
        }
    }
    if (!pdev->manufacturer->flashes[i].id) {
        goto unknown;
    }

    pdev->flash = &pdev->manufacturer->flashes[i];
    pdev->sector_size = pdev->flash->sector_size;
    pdev->num_sectors = pdev->flash->num_sectors;
    pdev->dummy_cycles = IFX_FAST_READ_DUMMY_BYTE;
    pdev->write_length = IFX_FLASH_PAGESIZE;
    
    pdev->size = pdev->sector_size * pdev->num_sectors;

    IFX_SFLASH_PRINT("SPI Device: %s 0x%02X (%s) 0x%02X 0x%02X\n"
        "Parameters: num sectors = %lu, sector size = %lu, write size = %u\n",
        pdev->flash->name, pdev->manufacturer_id, pdev->manufacturer->name,
        pdev->device_id1, pdev->device_id2, pdev->num_sectors,
        pdev->sector_size, pdev->write_length);    
    return 0;
unknown:
    printk("Unknown SPI device: 0x%02X 0x%02X 0x%02X\n",
        pdev->manufacturer_id, pdev->device_id1, pdev->device_id2);
    return -1;    
}

static INLINE int 
ifx_spi_flash_cs_handler(u32 csq, IFX_USIF_SPI_CS_DATA_t cs_data)
{
    if (csq == IFX_USIF_SPI_CS_ON) { /* Low active */
        return ifx_usif_spi_cs_low(cs_data);
    }
    else {
        return ifx_usif_spi_cs_high(cs_data);
    }
}

static INLINE void 
ifx_spi_flash_version(void)
{
    char ver_str[128] = {0};
    
    ifx_drv_ver(ver_str, "USIF SPI flash", IFX_USIF_SFLASH_VER_MAJOR, 
        IFX_USIF_SFLASH_VER_MID, IFX_USIF_SFLASH_VER_MINOR);
    printk(KERN_INFO "%s", ver_str);
} 

/**
 * \fn static int ifx_usif_flash_read(struct mtd_info *mtd, loff_t from, size_t len,
 *                 size_t *retlen ,u_char *buf)
 * \brief Read from the serial flash device.
 *
 * \param[in]  mtd    Pointer to struct mtd_info
 * \param[in]  from   Start offset in flash device
 * \param[in]  len    Amount to read
 * \param[out] retlen About of data actually read
 * \param[out] buf    Buffer containing the data
 * \return     0      No need to read actually or read successfully
 * \return     -EINVAL invalid read length
 * \ingroup IFX_USIF_SFLASH_OS
 */
static int
ifx_usif_flash_read(struct mtd_info *mtd, loff_t from, size_t len,
                  size_t *retlen ,u_char *buf)
{
    int total = 0;
    int len_this_lp;
    u32 addr;
    u8 *mem;
    
    IFX_SFLASH_PRINT("(from = 0x%.8x, len = %d)\n",(u32) from, (int)len);   
    if (!len)
        return 0;
    if ((from + len) > mtd->size)
        return -EINVAL;
    down(&ifx_sflash_sem);
    /* Fragment support */
    while (total < len) {
        mem              = (u8 *)(buf + total);
        addr             = from  + total;
        len_this_lp      = min((len - total), (size_t)IFX_USIF_SFLASH_FRAGSIZE);
        ifx_spi_read(addr, mem, len_this_lp);
        total += len_this_lp;
    }

    *retlen = len;
    up(&ifx_sflash_sem);
    return 0;
}

/**
 * \fn static int ifx_usif_flash_write(struct mtd_info *mtd, loff_t to, size_t len, 
 *                   size_t *retlen, const u_char *buf)
 * \brief Read from the serial flash device.
 *
 * \param[in]  mtd    Pointer to struct mtd_info
 * \param[in]  to   Start offset in flash device
 * \param[in]  len    Amount to write
 * \param[out] retlen Amount of data actually written
 * \param[out] buf    Buffer containing the data
 * \return     0      No need to read actually or read successfully
 * \return      -EINVAL invalid read length
 * \ingroup IFX_USIF_SFLASH_OS
 */
static int
ifx_usif_flash_write(struct mtd_info *mtd, loff_t to, size_t len, 
                    size_t *retlen, const u_char *buf)
{
    int total = 0, len_this_lp, bytes_this_page;
    u32 addr = 0;
    u8 *mem;
    
    IFX_SFLASH_PRINT("(to = 0x%.8x, len = %d)\n",(u32) to, len);

    if (retlen) {
        *retlen = 0;
    }
    
    /* sanity check */
    if (len == 0) {
        return 0;
    }
    
    if ((to + len) > mtd->size) {
        return (-1);
    }
    down(&ifx_sflash_sem);
    while(total < len) {
        mem              = (u8 *)(buf + total);
        addr             = to  + total;
        bytes_this_page  = spi_sflash->write_length - (addr % spi_sflash->write_length);
        len_this_lp      = min((len - total), (size_t)bytes_this_page);       
        ifx_spi_write(addr, mem, len_this_lp);
        total += len_this_lp;
    }
    *retlen = len;
    up(&ifx_sflash_sem);
    return 0;
}

/**
 * \fn static int ifx_usif_flash_erase(struct mtd_info *mtd,struct erase_info *instr)
 * \brief Erase pages of serial flash device.
 *
 * \param[in]  mtd    Pointer to struct mtd_info
 * \param[in]  instr  Pointer to struct erase_info
 * \return     0      OK
 * \return     -EINVAL invalid erase size
 * \ingroup IFX_USIF_SFLASH_OS
 */
static int
ifx_usif_flash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    u32 addr,len;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    IFX_SFLASH_PRINT("(addr = 0x%llx, len = %lld)\n", (long long)instr->addr, (long long)instr->len);
#else
    IFX_SFLASH_PRINT("(addr = 0x%.8x, len = %d)\n", instr->addr, instr->len);
#endif 
    if ((instr->addr + instr->len) > mtd->size) 
        return (-EINVAL);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    {
        uint32_t rem;
        div_u64_rem(instr->len, mtd->erasesize, &rem);
        if (rem) {
            return -EINVAL;
        }
    }
#else
    if ((instr->addr % mtd->erasesize) != 0
        /* || (instr->len % mtd->erasesize) != 0*/) {
        return -EINVAL;
    }
#endif
    addr = instr->addr;
    len = instr->len;

    down(&ifx_sflash_sem);
    /* whole-chip erase? */
    if (len == mtd->size) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)        
        IFX_SFLASH_PRINT("%lldKiB\n", (long long)(mtd->size >> 10));
#else
        IFX_SFLASH_PRINT("%ldKiB\n", (long)(mtd->size >> 10));
#endif 
        if (ifx_spi_chip_erase() != 0) {
            instr->state = MTD_ERASE_FAILED;
            up(&ifx_sflash_sem);
            return -EIO;
        }
    }
    else {
        /* REVISIT in some cases we could speed up erasing large regions
         * by using OPCODE_SE instead of OPCODE_BE_4K.  We may have set up
         * to use "small sector erase", but that's not always optimal.
         */
        while (len) {
            if (ifx_spi_sector_erase(addr) != 0) {
                instr->state = MTD_ERASE_FAILED;
                up(&ifx_sflash_sem);
                return -EIO;
            }
            addr += mtd->erasesize;
            len -= mtd->erasesize;
        }
    }
    up(&ifx_sflash_sem);
    
    /* Inform MTD subsystem that erase is complete */
    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);
    IFX_SFLASH_PRINT("return\n");
    return 0;
}

static INLINE IFX_USIF_SPI_HANDLE_t
ifx_spi_flash_register(char *dev_name) 
{
    IFX_USIF_SPI_CONFIGURE_t usif_cfg = {0};
    
    usif_cfg.baudrate     = IFX_USIF_SFLASH_BAUDRATE;
    usif_cfg.csset_cb     = ifx_spi_flash_cs_handler;
    usif_cfg.cs_data      = IFX_USIF_SFLASH_CS;
    usif_cfg.fragSize     = IFX_USIF_SFLASH_FRAGSIZE;
    usif_cfg.maxFIFOSize  = IFX_USIF_SFLASH_MAXFIFOSIZE;
    usif_cfg.spi_mode     = IFX_USIF_SFLASH_MODE;
    usif_cfg.spi_prio     = IFX_USIF_SFLASH_PRIORITY;
    usif_cfg.duplex_mode  = IFX_USIF_SPI_HALF_DUPLEX;
    return ifx_usif_spiAllocConnection(dev_name, &usif_cfg);
}

static INLINE int
ifx_spi_flash_size_to_index(u32 size)
{
    int i;
    int index = IFX_FLASH_128KB;
    
    i = (size >> 17); /* 128 KB minimum */
    if (i <= 1) {
        index = IFX_FLASH_128KB;
    }
    else if (i <= 2) {
        index = IFX_FLASH_256KB;
    }
    else if (i <= 4) {
        index = IFX_FLASH_512KB;
    }
    else if (i <= 8) {
        index = IFX_FLASH_1MB;
    }
    else if (i <= 16) {
        index = IFX_FLASH_2MB;
    }
    else if (i <= 32) {
        index = IFX_FLASH_4MB;
    }
    else if (i <= 64) {
        index = IFX_FLASH_8MB;
    }
    else if (i <= 128) {
        index = IFX_FLASH_16MB;
    }
    else if (i <= 256) {
        index = IFX_FLASH_32MB;
    }
    else {
        index = IFX_SPI_MAX_FLASH;
    }
    return index;
}

static INLINE void 
ifx_spi_flash_gpio_init(void)
{
    ifx_gpio_register(IFX_GPIO_MODULE_USIF_SPI_SFLASH);
}

static INLINE void 
ifx_spi_flash_gpio_release(void)
{   
    ifx_gpio_deregister(IFX_GPIO_MODULE_USIF_SPI_SFLASH);
}

static int __init 
ifx_usif_spi_flash_init (void)
{
    IFX_USIF_SPI_HANDLE_t *sflash_handler;
    struct mtd_info *mtd;
#ifdef CONFIG_MTD_CMDLINE_PARTS
    int np;
#endif /* CONFIG_MTD_CMDLINE_PARTS */
    int ret = 0;
    int index;

    sema_init(&ifx_sflash_sem, 1);
    ifx_spi_flash_gpio_init();
    spi_sflash = (ifx_usif_spi_dev_t *)kmalloc(sizeof(ifx_usif_spi_dev_t), GFP_KERNEL);
    if (spi_sflash == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    memset(spi_sflash, 0, sizeof (ifx_usif_spi_dev_t));
    /* 
     * Make sure tx buffer address is DMA burst length aligned and 2 page size< 512> should be enouhg for
     * serial flash. In this way, host cpu can make good use of DMA operation.
     */    
    spi_sflash->flash_tx_org_buf = kmalloc(IFX_SFLASH_MAX_WRITE_SIZE + IFX_SFLASH_DMA_MAX_BURST_LEN - 1, GFP_KERNEL);
    if (spi_sflash->flash_tx_org_buf == NULL) {
        ret = -ENOMEM;
        goto err1;
    }
    spi_sflash->flash_tx_buf = (char *)(((u32)(spi_sflash->flash_tx_org_buf + IFX_SFLASH_DMA_MAX_BURST_LEN - 1))
                                & ~(IFX_SFLASH_DMA_MAX_BURST_LEN - 1));

    /* 
     * Make sure rx buffer address is DMA burst length aligned and 8 page size< 2KB> should be enouhg for
     * serial flash. In this way, host cpu can make good use of DMA operation.
     */
    spi_sflash->flash_rx_org_buf = kmalloc(IFX_SFLASH_MAX_READ_SIZE + IFX_SFLASH_DMA_MAX_BURST_LEN - 1, GFP_KERNEL);
    if (spi_sflash->flash_rx_org_buf == NULL) {
        ret = -ENOMEM;
        goto err2;
    }
    spi_sflash->flash_rx_buf = (char *)(((u32)(spi_sflash->flash_rx_org_buf + IFX_SFLASH_DMA_MAX_BURST_LEN - 1))
                                & ~(IFX_SFLASH_DMA_MAX_BURST_LEN - 1));

    mtd = kmalloc(sizeof(struct mtd_info), GFP_KERNEL);
    if ( mtd == NULL ) {
        printk(KERN_WARNING "%s Cant allocate mtd stuff\n", __func__);
        ret = -ENOMEM;
        goto err3;
    }
    memset(mtd, 0, sizeof(struct mtd_info));
    
    sflash_handler = ifx_spi_flash_register(IFX_USIF_SFLASH_NAME);
    if (sflash_handler == NULL) {
        printk("%s: failed to register sflash\n", __func__);
        ret = -ENOMEM;
        goto err4;
    }   
  
    spi_sflash->sflash_handler = sflash_handler;
    if (ifx_usif_flash_probe(spi_sflash) != 0) {
        printk(KERN_WARNING "%s: Found no serial flash device\n", __func__);
        ret = -ENXIO;
        goto err5;
    }

    if (mtd->size > 0x1000000) {
        spi_sflash->addr_cycles = 4;
        ifx_spi_set_4byte(spi_sflash, 1);
    }
    else {
        spi_sflash->addr_cycles = IFX_SFLASH_ADDR_CYCLES;
    }

    mtd->name               =   IFX_USIF_SFLASH_NAME;
    mtd->type               =   MTD_NORFLASH;
    mtd->flags              =   (MTD_CAP_NORFLASH | MTD_WRITEABLE); 
    mtd->size               =   spi_sflash->size;
    /* Prefer "small sector" erase if possible */
    if (spi_sflash->flash->flags & SECT_4K) {
        spi_sflash->erase_opcode = IFX_OPCODE_BE_4K;
        mtd->erasesize           = 4096;
    } else {
        spi_sflash->erase_opcode = IFX_OPCODE_SE;
        mtd->erasesize           =  spi_sflash->sector_size;
    }    

    mtd->numeraseregions    =   0;
    mtd->eraseregions       =   NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,41)
    mtd->module             =   THIS_MODULE;
#else
    mtd->owner              =   THIS_MODULE;
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16)
    mtd->writesize          =   1;   /* like NOR flash, should be 1 */
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
    mtd->_erase              =   ifx_usif_flash_erase;
    mtd->_read               =   ifx_usif_flash_read;
    mtd->_write              =   ifx_usif_flash_write;
#else
    mtd->erase              =   ifx_usif_flash_erase;
    mtd->read               =   ifx_usif_flash_read;
    mtd->write              =   ifx_usif_flash_write;
#endif
    index                   =   ifx_spi_flash_size_to_index(spi_sflash->size);
    if (index > IFX_SPI_MAX_FLASH) {
        printk (KERN_WARNING "%s: flash size is too big to support\n", __func__);
        ret = -EINVAL;
        goto err5;
    }
#ifdef IFX_SPI_FLASH_DBG
    printk (KERN_DEBUG
            "mtd->name = %s\n"
            "mtd->size = 0x%.8x (%uM)\n"
            "mtd->erasesize = 0x%.8x (%uK)\n"
            "mtd->numeraseregions = %d\n"
            "mtd index %d\n",
            mtd->name,
            mtd->size, mtd->size / (1024*1024),
            mtd->erasesize, mtd->erasesize / 1024,
            mtd->numeraseregions, index);

    if (mtd->numeraseregions) {
        int result;

        for (result = 0; result < mtd->numeraseregions; result++) {
            printk (KERN_DEBUG
                "\n\n"
                "mtd->eraseregions[%d].offset = 0x%.8x\n"
                "mtd->eraseregions[%d].erasesize = 0x%.8x (%uK)\n"
                "mtd->eraseregions[%d].numblocks = %d\n",
                result, mtd->eraseregions[result].offset,
                result, mtd->eraseregions[result].erasesize,
                mtd->eraseregions[result].erasesize / 1024,
                result, mtd->eraseregions[result].numblocks);
         }
    }
#endif  /* IFX_SPI_FLASH_DBG */

#ifdef CONFIG_MTD_CMDLINE_PARTS
    np = parse_mtd_partitions(mtd, part_probes, &spi_sflash->parsed_parts, 0);
    if (np > 0) {
        add_mtd_partitions(mtd, spi_sflash->parsed_parts, np);
    }
    else {
        printk(KERN_ERR "%s: No valid partition table found in command line\n", __func__);
        goto err5;
    }
#else
    add_mtd_partitions(mtd, g_ifx_mtd_spi_partitions[index], ARRAY_SIZE(g_ifx_mtd_spi_partitions[index]));
#endif /* CONFIG_MTD_CMDLINE_PARTS */

    spi_sflash->mtd = mtd;
    ifx_spi_flash_version();
    return ret;
err5:
    ifx_usif_spiFreeConnection(spi_sflash->sflash_handler);
err4:
    kfree(mtd);
err3:
    kfree(spi_sflash->flash_rx_org_buf);
err2:
    kfree(spi_sflash->flash_tx_org_buf);
err1:
    kfree(spi_sflash);
done:
    ifx_spi_flash_gpio_release();
    return ret;
}

static void __exit 
ifx_usif_spi_flash_exit(void)
{
    if (spi_sflash != NULL) {
        if (spi_sflash->parsed_parts != NULL) {
            del_mtd_partitions (spi_sflash->mtd);
        }
        kfree(spi_sflash->mtd);
        ifx_usif_spiFreeConnection(spi_sflash->sflash_handler);
        kfree(spi_sflash->flash_rx_org_buf);
        kfree(spi_sflash->flash_tx_org_buf);
        kfree(spi_sflash);
    }
    ifx_spi_flash_gpio_release();
}
module_init(ifx_usif_spi_flash_init);
module_exit(ifx_usif_spi_flash_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chuanhua.Lei@infineon.com");
MODULE_SUPPORTED_DEVICE("Serial flash 25 types generic driver");
MODULE_DESCRIPTION("IFAP USIF SPI flash device driver");

