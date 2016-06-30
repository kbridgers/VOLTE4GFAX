/******************************************************************************
**
** FILE NAME    : ifxmips_usif_uart.c
** PROJECT      : IFX UEIP
** MODULES      : Serial driver for USIF
**
** DATE         : 09 May 2009
** AUTHOR       : Lei Chuanhua
** DESCRIPTION  : USIF module for uart driver
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
** HISTORY
** $Date        $Author         $Comment
** 15 May,2009  Lei Chuanhua    Verififed on RTL REL 26p5/RTL 28.
*******************************************************************************/
/*!
  \defgroup IFX_USIF_UART  IFX USIF UART module
  \brief ifx usif uart mode driver module
*/

/*!
  \defgroup IFX_USIF_UART_DEFINITIONS Defintion and structures
  \ingroup IFX_USIF_UART
  \brief definitions for ifx usif uart mode driver 
*/

/*!
  \defgroup IFX_USIF_UART_FUNCTIONS OS APIs
  \ingroup  IFX_USIF_UART
  \brief ifx usif uart driver external functions
*/

/*!
  \defgroup IFX_USIF_UART_INTERNAL  Internal functions
  \ingroup  IFX_USIF_UART
  \brief ifx usif uart driver internal functions
*/

/*!
  \file ifxmips_usif_uart.c
  \ingroup IFX_USIF_UART
  \brief usif uart driver source file
*/

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif
#include <linux/module.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/io.h>

/* Project header */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>

#include "ifxmips_usif_uart.h"

#define IFX_USIF_UART_VER_MAJOR          1
#define IFX_USIF_UART_VER_MID            1
#define IFX_USIF_UART_VER_MINOR          4

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
#define UART_PORT_TO_TTY(port)  ((port)->state->port.tty)
#define UART_PORT_TO_XMIT(port) &((port)->state->xmit)
#define UART_PORT_TO_MSR(port)  ((port)->state->port.delta_msr_wait)
#else
#define UART_PORT_TO_TTY(port)  ((port)->info->tty)
#define UART_PORT_TO_XMIT(port) &((port)->info->xmit)
#define UART_PORT_TO_MSR(port)  ((port)->info->delta_msr_wait)
#endif

//#define IFX_USIF_UART_DEBUG

enum {
    USIF_UART_MSG_TX     = 0x00000001,
    USIF_UART_MSG_RX     = 0x00000002,
    USIF_UART_MSG_INT    = 0x00000004,  /* Interrupt msg */
    USIF_UART_MSG_CFG    = 0x00000008,
    USIF_UART_MSG_DEBUG  = 0x00000010,
    USIF_UART_MSG_ERROR  = 0x00000020,
    USIF_UART_MSG_INIT   = 0x00000040,
    USIF_UART_MSG_CONSOLE = 0x00000080,
    USIF_UART_MSG_ANY    = 0x000000FF,   /* anything */
};

#ifdef IFX_USIF_UART_DEBUG
#define INLINE 
static void ifx_usif_uart_debug(IFX_USIF_UART_PORT_t *port, const char *fmt, ...);

#define IFX_USIF_UART_PRINT(_port, _m, _fmt, args...) do { \
    if ((_port)->debug & (_m)) {                 \
        ifx_usif_uart_debug((_port), (_fmt), ##args);      \
    }                                                \
} while (0)

#else
#define INLINE  inline
#define IFX_USIF_UART_PRINT(_port, _m, _fmt, ...)
#endif /* IFX_USIF_DEBUG */

static int ifx_usif_uart_hw_initialized = 0;

static void ifx_usif_uart_stop_tx (struct uart_port *port);

extern unsigned int ifx_get_usif_hz(void);

#if defined(CONFIG_IFX_USIF_UART_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#if defined(CONFIG_USE_PALLADIUM)
#define IFX_USIF_UART_MAX_BAUDRATE         4800
#define IFX_USIF_UART_DEFAULT_BAUDRATE     IFX_USIF_BAUD_2400
#elif defined(CONFIG_USE_VENUS)
#define IFX_USIF_UART_MAX_BAUDRATE         57600
#define IFX_USIF_UART_DEFAULT_BAUDRATE     IFX_USIF_BAUD_38400
#else  
#define IFX_USIF_UART_MAX_BAUDRATE         115200
#define IFX_USIF_UART_DEFAULT_BAUDRATE     CONFIG_IFX_USIF_UART_DEFAULT_BAUDRATE
#endif /* CONFIG_USE_PALLADIUM */

/* XXX, adjust the following table according to uart_clk clock */
static const IFX_USIF_HW_BAUDTABLE_t ifx_usif_uart_baudrate_table[] = {
    /* baud table index 0, for palladium  75638 */
    { IFX_USIF_UART_MAX_BAUDRATE,

    {
    { 19, 292, 0, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_300},     /*     300 baud */
    { 97, 300, 0, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_1200 },   /*    1200 baud */
    { 194,203, 0, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_2400},    /*    2400 baud */
    { 474,111,0, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_4800 },   /*    4800 baud */
    /* The following is placeholder */    
    { 1,  1,  71,1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_9600 },   /*    9600 baud */
    { 1,  1,  35,1, 1, 10, 0,  IFX_USIF_HW_SHORT_TIMEOUT_19200 },  /*   19200 baud  */
    { 1,  1,  11,1, 1, 20, 0,  IFX_USIF_HW_SHORT_TIMEOUT_38400 },  /*   38400 baud */
    { 1,  1,  11,1, 1, 29, 0,  IFX_USIF_HW_SHORT_TIMEOUT_57600 },  /*   57600 baud */
    { 1,  1,  5, 1, 2, 58, 0,  IFX_USIF_HW_SHORT_TIMEOUT_115200 }, /*  115200 baud */
    }
    },

    /* baud table index 1 for Venus 1000000 */
    { IFX_USIF_UART_MAX_BAUDRATE,
    {
    { 3, 2, 124, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_300},     /*     300 baud */
    { 12,13, 24, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_1200 },   /*    1200 baud */
    { 24,1, 24, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_2400},    /*    2400 baud */
    { 48,77, 4, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_4800 },   /*    4800 baud */
    { 96,29, 4, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_9600 },   /*    9600 baud */
    { 192,433,0,1, 1, 10, 0,  IFX_USIF_HW_SHORT_TIMEOUT_19200 },  /*   19200 baud  */
    { 384,241,0,1, 1, 20, 0,  IFX_USIF_HW_SHORT_TIMEOUT_38400 },  /*   38400 baud */
    { 482,41, 0,1, 1, 29, 0,  IFX_USIF_HW_SHORT_TIMEOUT_57600 },  /*   57600 baud */
    /* The following is placeholder */  
    { 1,  1,  5, 1, 2, 58, 0,  IFX_USIF_HW_SHORT_TIMEOUT_115200 }, /*  115200 baud */
    }
    },

    /* baud table index 2 for silicon 100000000 */
    { IFX_USIF_UART_MAX_BAUDRATE,
    {
    { 1, 2, 6921, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_300},     /*     300 baud */
    { 1, 346, 14, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_1200 },   /*    1200 baud */
    { 1, 185, 13, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_2400 },    /*    2400 baud */
    { 1, 92, 13, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_4800 },   /*    4800 baud */
    { 1,92,  6, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_9600 },   /*    9600 baud */
    { 25,601, 12, 1, 1, 10, 0,  IFX_USIF_HW_SHORT_TIMEOUT_19200 },  /*   19200 baud  */
    { 25,288, 12, 1, 1, 20, 0,  IFX_USIF_HW_SHORT_TIMEOUT_38400 },  /*   38400 baud */
    { 75,551,12, 1, 1, 29, 0,  IFX_USIF_HW_SHORT_TIMEOUT_57600 },  /*   57600 baud */
    { 75,238,12, 1, 2, 58, 0,  IFX_USIF_HW_SHORT_TIMEOUT_115200 }, /*  115200 baud */
    }
    },
    
    /* baud table index 3 for silicon 20000000 */
    { IFX_USIF_UART_MAX_BAUDRATE,
    {
    { 1, 259, 15,  1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_300},     /*     300 baud */
    { 1, 346,2,1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_1200 },   /*    1200 baud */
    { 3, 139,10, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_2400 },    /*    2400 baud */
    { 5, 88, 13, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_4800 },   /*    4800 baud */
    { 5, 88, 6, 1, 1, 5,  0,  IFX_USIF_HW_SHORT_TIMEOUT_9600 },   /*    9600 baud */
    { 29,89, 15, 1, 1, 10, 0,  IFX_USIF_HW_SHORT_TIMEOUT_19200 },  /*   19200 baud  */
    { 29,30, 15, 1, 1, 20, 0,  IFX_USIF_HW_SHORT_TIMEOUT_38400 },  /*   38400 baud */
    { 77,480,2, 1, 1, 29, 0,  IFX_USIF_HW_SHORT_TIMEOUT_57600 },  /*   57600 baud */
    { 154,403,2, 1, 2, 58, 0,  IFX_USIF_HW_SHORT_TIMEOUT_115200 }, /*  115200 baud */
    }
    },
};

static struct proc_dir_entry *ifx_usif_uart_proc = NULL;
static const char ifx_usif_uart_proc_node_name[] = "driver/ifx_usif_uart";
static int ifx_usif_uart_proc_read(char *page, char **start, off_t off, int count, int *eof, void *dev);


#ifdef IFX_USIF_UART_DEBUG
extern void prom_printf(const char * fmt, ...);

/**
 * \fn static void ifx_usif_uart_debug(IFX_USIF_UART_PORT_t *port, const char *fmt, ...)
 * \brief Debug all kinds of level message
 *
 * \param   port    Pointer to structure #ifx_usif_uart_debug
 * \param   fmt     debug output format
 * \ingroup IFX_USIF_UART_INTERNAL             
 */
static void
ifx_usif_uart_debug(IFX_USIF_UART_PORT_t *port, const char *fmt, ...)
{
    static char buf[256] = {0};      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap); 

    printk("%s\n", buf);
}

/**
 * \fn static void  ifx_usif_uart_reg_dump(struct uart_port *port)
 * \brief Call this function to dump register through prom_printf
 *
 * \param   port    Pointer to structure #usif_uart
 * \return  none
 * \ingroup IFX_USIF_UART_INTERNAL             
 */
static void 
ifx_usif_uart_reg_dump(struct uart_port *port)
{
    prom_printf("USIF_ID           0x%08x\n", IFX_USIF_GET_ID(port));
    prom_printf("USIF_SWCID        0x%08x\n", IFX_USIF_GET_SWC_ID(port));
    prom_printf("USIF_FIFO_ID      0x%08x\n", IFX_USIF_GET_FIFO_ID(port));

    prom_printf("USIF_CLC_CNT      0x%08x\n", IFX_USIF_GET_CLOCK_COUNTER(port));
    prom_printf("USIF_CLC_STAT     0x%08x\n", IFX_USIG_GET_CLOCK_STATUS(port));

    prom_printf("USIF_MODE_CFG     0x%08x\n", IFX_USIF_GET_MODE_CONFIG(port));
    prom_printf("USIF_PRTC_CFG     0x%08x\n", IFX_USIF_GET_PROTOCOL_CONFIG(port));
    prom_printf("USIF_PRTC_STAT    0x%08x\n", IFX_USIF_GET_PROTOCOL_STATUS(port));

    prom_printf("USIF_CS_CFG       0x%08x\n", IFX_USIF_GET_CHIPSELECT_CONFIG(port));
    prom_printf("USIF_FDIV_CFG     0x%08x\n", IFX_USIF_GET_FRAC_DIV(port));
    prom_printf("USIF_BC_CFG       0x%08x\n", IFX_USIF_GET_BAUDRATE_COUNTER(port));
    prom_printf("USIF_ICTMO_CFG    0x%08x\n", IFX_USIF_GET_INTERCHARACTER_TIMEOUT(port));

    prom_printf("USIF_FIFO_CFG     0x%08x\n", IFX_USIF_GET_FIFO_CONFIG(port));
    prom_printf("USIF_FIFO_STAT    0x%08x\n", IFX_USIF_GET_FIFO_STATUS(port));
    prom_printf("USIF_TXD_SB       0x%08x\n", IFX_USIF_GET_TXD_SIDEBAND_CONFIG(port));
    prom_printf("USIF_DPLUS_STAT   0x%08x\n", IFX_USIF_GET_DPLUS_STATUS(port));
    prom_printf("USIF_TXD          0x%08x\n", IFX_USIF_GET_TX_WORD(port));
    if (IFX_USIG_GET_CLOCK_STATUS(port) & IFX_USIF_CLC_STAT_RUN) {
        prom_printf("USIF_RXD          0x%08x\n", IFX_USIF_GET_RX_WORD(port));
    }
    prom_printf("USIF_MRPS_CTRL    0x%08x\n", IFX_USIF_GET_RX_MRPS(port));
    prom_printf("USIF_RIS          0x%08x\n", IFX_USIF_GET_RAW_INT_STATUS(port));
    prom_printf("USIF_IMSC         0x%08x\n", IFX_USIF_GET_INT_MASK(port));
    prom_printf("USIF_MIS          0x%08x\n", IFX_USIF_GET_INT_STATUS(port));
}

#endif /* IFX_USIF_DEBUG */

/**
 * \fn     static unsigned int ifx_usif_uart_rx_err(struct uart_port *port, u8 *flg)
 *
 * \brief  Helper function receive interrupts, to check any error occured.
 *
 *        The return value of this function tells whether to ignore the receiveed 
 *        character or not, where as the 'flg' is updated with the received character
 *        is normal or contain any error.
 
 * \param  port   pointer to USIF uart port.
 * \param   flg    pointer, this function updates it with appropriate flag.
 *
 * \return  USIF_UART_RX_GOOD_CHAR    Need to insert  the data to FLIP buffer 
 * \return  USIF_UART_RX_IGNORE_CHAR  Ignore the received data
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static unsigned int
ifx_usif_uart_rx_err(struct uart_port *port, u8 *flg)
{
    u32 ret;
    u32 error_state;
    struct tty_struct *tty = UART_PORT_TO_TTY(port);

    /* The 'flg' is set to normal, if no error detected */
    *flg = TTY_NORMAL;
    ret = IFX_USIF_UART_RX_GOOD_CHAR;

    /*
     * USIF supports RX overflow error, parity error and frame error.
     * It does not support the break error 
     */
    error_state = IFX_USIF_GET_INT_STATUS(port) & IFX_USIF_RX_ERR;
    if (error_state) {
        if ((error_state & IFX_USIF_INT_RXOF)) {
            port->icount.overrun++;
            IFX_USIF_UART_IRQ_LOCK(port);
            IFX_USIF_CLR_INT(IFX_USIF_INT_RXOF, port);
            IFX_USIF_UART_IRQ_UNLOCK(port);
        }
        if ((error_state & IFX_USIF_INT_PE)) {
            port->icount.parity++;
            IFX_USIF_UART_IRQ_LOCK(port);
            IFX_USIF_CLR_INT(IFX_USIF_INT_PE, port);
            IFX_USIF_UART_IRQ_UNLOCK(port);
        }
        if ((error_state & IFX_USIF_INT_FE)) {
            port->icount.frame++;
            IFX_USIF_UART_IRQ_LOCK(port);
            IFX_USIF_CLR_INT(IFX_USIF_INT_FE, port);
            IFX_USIF_UART_IRQ_UNLOCK(port);
        }
 
        /* Check whether we need to ignore the characters */
        if (error_state & port->ignore_status_mask) {
            return IFX_USIF_UART_RX_IGNORE_CHAR;
        }
 
        /* Check whether we need to read the characters with errors */
        error_state &= port->read_status_mask;
        if (error_state & IFX_USIF_INT_PE) {
            *flg = TTY_PARITY;
        }
        else if (error_state & IFX_USIF_INT_FE) {
            *flg = TTY_FRAME;
        }
 
        if (error_state & IFX_USIF_INT_RXOF) {
            /*
             * If we have overrun error, report it immediately, as it does not 
             * affect the received characters 
             */
            tty_insert_flip_char(tty, 0, TTY_OVERRUN);
        }
    }
    /* Else do nothing, no error detected */
    return ret;
}

/**
 * \fn     static INLINE void  ifx_usif_uart_run_time_config(struct uart_port *port)
 *
 * \brief  Call this function to reset some runtime parameters
 *
 * \param  port   pointer to USIF uart port.
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static INLINE void 
ifx_usif_uart_run_time_config(struct uart_port *port)
{
    u32 reg;
    
    /* 
     * NB, Configure RX MRPS to zero, so intercharacter timeout must be set 
     * This is important, otherwise, rx interrupt may not generate.
     * MRPS should be configured for every rx transaction
     * Once config/run mode switched, MRPS will be reset to zero
     */
    IFX_USIF_CONFIG_RX_MRPS(0, port);

    /* TX sideband register, class 0, no DMA for RX/TX */
    IFX_USIF_CONFIG_TXD_SIDEBAND(SM(IFX_USIF_TXD_CLASS0, IFX_USIF_TXD_SB_TX_CLS), port);
    reg = IFX_USIF_DPLUS_CTRL_TX_DIS | IFX_USIF_DPLUS_CTRL_RX_MASK | IFX_USIF_DPLUS_CTRL_SET_MASK;
    IFX_USIF_CONFIG_DPLUS_CONTROL(reg, port);
}

/**
 * \fn     static void ifx_usif_uart_rx_breq(struct uart_port *port)
 *
 * \brief  Receive interrupt service routine for BREQ.
 *
 *        The TOPSPIN Receive FIFO is configured as Flow-Controller mode, 
 *        byte aligned and burst size is set to four words. So, each received 
 *        word will have four characters 
 *
 * \param  port   pointer to USIF uart port.
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void
ifx_usif_uart_rx_breq(struct uart_port *port)
{
    u8 flg;
    struct tty_struct *tty = UART_PORT_TO_TTY(port);
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    IFX_USIF_RXD_t rxd_data;
    u32 word_cnt, char_cnt;

    /* Read each words in the whole burst */
    for (word_cnt = 0; word_cnt < IFX_USIF_UART_RX_BUSRT_SIZE; word_cnt++) {
        
        stat->rx_read_fifo++;
 
        /* Read the word from the RX FIFO, update the local RPS counter and RX counter */
        rxd_data.rxd_word = IFX_USIF_GET_RX_WORD(port);
        usif_port_p->rx_rps_cnt += IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD;
        port->icount.rx += IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD;
  
        /*
         *  Check whether any error set. If any error set, check whether the
         *  character need to be ignored, using ignore_status_mask and read_status_mask.
         *  Note that, we ignore the whole word (4 characters) 
         */
        if (ifx_usif_uart_rx_err(port, &flg) != IFX_USIF_UART_RX_IGNORE_CHAR) {
            for (char_cnt = 0; char_cnt < IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD; char_cnt++) {
                unsigned char rx_byte = rxd_data.rxd_byte.byte[char_cnt];

                if (!uart_handle_sysrq_char(port, rx_byte)) {
                    tty_insert_flip_char(tty, rx_byte, flg);
                }
            }
        }
    }
    stat->rx_intr_breq++;
    /*
     * Try to push the data from FLIP buffer to line discipline. This need not be required
     * in case of low latency enabled as it does that while checking the sufficient space
     * in FLIP buffer initially 
     */
    if (!(tty->low_latency)) {
        tty_flip_buffer_push(tty);
    }
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT(IFX_USIF_INT_RX_BREQ, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static void ifx_usif_uart_rx_lbreq(struct uart_port *port)
 *
 * \brief  Receive interrupt service routine for LBREQ.
 *
 * \param  port   pointer to USIF uart port.
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void
ifx_usif_uart_rx_lbreq(struct uart_port *port)
{
    char flg;
    struct tty_struct *tty =  UART_PORT_TO_TTY(port);
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    u32 word_cnt, char_cnt;
    u32 ign_char;
    u32 rx_cnt = 0;
    u32 reg;
    IFX_USIF_RXD_t rxd_data;

    /* Receive complete words first */
    for (word_cnt = 0; word_cnt < IFX_USIF_UART_RX_BUSRT_SIZE - 1; word_cnt++){
  
        stat->rx_read_fifo++;
        rxd_data.rxd_word = IFX_USIF_GET_RX_WORD(port);
  
        /*
         * Check whether any error set. If any error set, check whether the
         * character need to be ignored, using ignore_status_mask and read_status_mask.
         * Note that, we ignore the whole word (4 characters)
         */
        ign_char = ifx_usif_uart_rx_err(port, &flg);
   
        for (char_cnt = 0; char_cnt < IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD; char_cnt++) {
            usif_port_p->rx_rps_cnt++;
            port->icount.rx++;
            if (ign_char != IFX_USIF_UART_RX_IGNORE_CHAR) {
                unsigned char rx_byte = rxd_data.rxd_byte.byte[char_cnt];

                if (!uart_handle_sysrq_char(port, rx_byte)) {
                    tty_insert_flip_char(tty, rx_byte, flg);
                }
            }
        }
    }

    /* Receive the last complete or incomplete word */
    stat->rx_read_fifo++;
    reg = IFX_USIF_GET_DPLUS_STATUS(port);
    if (reg & IFX_USIF_DPLUS_STAT_RME) {
        rx_cnt = MS(reg, IFX_USIF_DPLUS_STAT_RX_BE);
        if (rx_cnt == 0) {
            rx_cnt = IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD;
        }
    }
    rxd_data.rxd_word = IFX_USIF_GET_RX_WORD(port); 
    /*
     * Check wh1ether any error set. If any error set, check whether the
     * character need to be ignored, using ignore_status_mask and read_status_mask.
     *  Note that, we ignore the whole word (4 chracters)
     */
    ign_char = ifx_usif_uart_rx_err(port, &flg);
  
    for (char_cnt = 0; char_cnt < rx_cnt; char_cnt++) {
        usif_port_p->rx_rps_cnt++;
        port->icount.rx++;
        if (ign_char != IFX_USIF_UART_RX_IGNORE_CHAR) {
            unsigned char rx_byte = rxd_data.rxd_byte.byte[char_cnt];
            
            if (!uart_handle_sysrq_char(port, rx_byte)) {
                tty_insert_flip_char(tty, rx_byte, flg);
            }
        }
    }
    
    stat->rx_intr_lbreq++;

    /* Completed the reception of packet, clear this count */
    usif_port_p->rx_rps_cnt = 0x00;
 
    /* Try to push the data from FLIP buffer to line discipline. */
    tty_flip_buffer_push(tty);
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT(IFX_USIF_INT_RX_LBREQ, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static void ifx_usif_uart_rx_sreq(struct uart_port *port)
 *
 * \brief  Receive interrupt service routine for SREQ.
 *
 
 * \param  port   pointer to USIF uart port.
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void
ifx_usif_uart_rx_sreq(struct uart_port *port)
{
    u8 flg;
    struct tty_struct *tty =  UART_PORT_TO_TTY(port);
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    u32 char_cnt;
    IFX_USIF_RXD_t rxd_data;
 
    stat->rx_read_fifo++;
    rxd_data.rxd_word = IFX_USIF_GET_RX_WORD(port);
    usif_port_p->rx_rps_cnt += IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD;
    port->icount.rx += IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD;

#ifdef IFX_USIF_UART_DEBUG
{
    int i;

    /* XXX, dump the data to verify  */
    prom_printf("%s rxd_data.rxd_word 0x%08x\n", __func__, rxd_data.rxd_word);
    prom_printf("USIF_DPLUS_STAT 0x%08x\n", IFX_USIF_GET_DPLUS_STATUS(port));
    
    for (i = 0; i < 4; i++) {
        prom_printf("rxd_byte.byte[%d], %c\n", i, rxd_data.rxd_byte.byte[i]);
    }
}
#endif

    /*
     * Check whether any error set. If any error set, check whether the
     * character need to be ignored, using ignore_status_mask and read_status_mask.
     * Note that, we ignore the whole word (4 bytes)
     */
    if (ifx_usif_uart_rx_err(port, &flg) != IFX_USIF_UART_RX_IGNORE_CHAR) {
        for (char_cnt = 0; char_cnt < IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD; char_cnt++) {
            unsigned char rx_byte = rxd_data.rxd_byte.byte[char_cnt];

            if (!uart_handle_sysrq_char(port, rx_byte)) {
                tty_insert_flip_char(tty, rx_byte, flg);
            }
        }
     }
 
     stat->rx_intr_sreq++;
    /*
     * We need not try to push the data from FLIP buffer to line dicsipline here, 
     * as, at the most, we can get only 3 single requests in a row and the fourth
     * one would be the last single request
     */
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT(IFX_USIF_INT_RX_SREQ, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static void ifx_usif_uart_rx_lsreq(struct uart_port *port)
 *
 * \brief  Receive interrupt service routine for LSREQ.
 *
 * \param  port   pointer to USIF uart port.    
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void
ifx_usif_uart_rx_lsreq(struct uart_port *port)
{
    u8 flg;
    struct tty_struct *tty =  UART_PORT_TO_TTY(port);
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    u32 char_cnt;
    u32 ign_char;
    u32 rx_cnt = 0;
    IFX_USIF_RXD_t rxd_data;
    u32 reg;

    stat->rx_read_fifo++;
    
    reg = IFX_USIF_GET_DPLUS_STATUS(port);
    if (reg & IFX_USIF_DPLUS_STAT_RME) {
        rx_cnt = MS(reg, IFX_USIF_DPLUS_STAT_RX_BE);
        if (rx_cnt == 0) {
            rx_cnt = IFX_USIF_UART_RX_FIFO_CHARS_PER_WORD;
        }
    }
    
    rxd_data.rxd_word = IFX_USIF_GET_RX_WORD(port);

#ifdef IFX_USIF_UART_DEBUG
{
    int i;

    /* XXX, dump the data to verify  */
    prom_printf("%s rxd_data.rxd_word 0x%08x\n", __func__, rxd_data.rxd_word);
    prom_printf("USIF_DPLUS_STAT 0x%08x\n", IFX_USIF_GET_DPLUS_STATUS(port));
    
    for (i = 0; i < rx_cnt; i++) {
        prom_printf("rxd_byte.byte[%d], %c\n", i, rxd_data.rxd_byte.byte[i]);
    }
}
#endif
    /*
     * Check wh1ether any error set. If any error set, check whether the
     * character need to be ignored, using ignore_status_mask and read_status_mask.
     *  Note that, we ignore the whole word (4 chracters)
     */
    ign_char = ifx_usif_uart_rx_err(port, &flg);
  
    for (char_cnt = 0; char_cnt < rx_cnt; char_cnt++) {
        usif_port_p->rx_rps_cnt++;
        port->icount.rx++;
        if (ign_char != IFX_USIF_UART_RX_IGNORE_CHAR) {
            unsigned char rx_byte = rxd_data.rxd_byte.byte[char_cnt];
            
            if (!uart_handle_sysrq_char(port, rx_byte)) {
                tty_insert_flip_char(tty, rx_byte, flg);
            }
        }
    }
    stat->rx_intr_lsreq++;
    /* Completed the reception of packet, clear this count */
    usif_port_p->rx_rps_cnt = 0x00;
 
    tty_flip_buffer_push(tty);
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT(IFX_USIF_INT_RX_LSREQ, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static int ifx_usif_uart_put_tx_word(struct uart_port *port)
 *
 * \brief  Helper function transmit interrupts, to put the word into FIFO.
 *
 *
 * \param  port   pointer to USIF uart port.
 *            
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static int
ifx_usif_uart_put_tx_word(struct uart_port *port)
{
    int xme = 0;   /* Transmit Message End */
    int xbe = 0;   /* Transmit Byte Enable */
    u32 tx_tps_cnt;    
    struct circ_buf *xmit = UART_PORT_TO_XMIT(port);
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    IFX_USIF_TXD_t txd_data;
    u32 char_cnt;

    tx_tps_cnt = uart_circ_chars_pending(xmit);
    
    /* Sanity check, return immediately */
    if (tx_tps_cnt == 0) {
        IFX_USIF_CLR_INT_MASK(IFX_USIF_INT_TRANSMIT, port);
        return 0;
    }
    
    /* Last word or last incomplete word processing */
    if (tx_tps_cnt <= IFX_USIF_UART_TX_FIFO_CHARS_PER_WORD) {
        xme = 1;
        xbe = (tx_tps_cnt == IFX_USIF_UART_TX_FIFO_CHARS_PER_WORD)? 
            0: tx_tps_cnt;
    }
 
    for (char_cnt = 0; char_cnt < IFX_USIF_UART_TX_FIFO_CHARS_PER_WORD; char_cnt++) {
        if (tx_tps_cnt) {
    #ifdef IFX_USIF_UART_DEBUG   
            prom_printf("%s 0x%02x %c\n", __func__, xmit->buf[xmit->tail], xmit->buf[xmit->tail]);
    #endif
            txd_data.txd_byte.byte[char_cnt] = xmit->buf[xmit->tail];
            xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
            port->icount.tx++;
            tx_tps_cnt--;
        }
        else {
            IFX_USIF_CLR_INT_MASK(IFX_USIF_INT_TRANSMIT, port);
            break;
        }
    }
    IFX_USIF_UART_IRQ_LOCK(port);
    /* XXX, Last Complete/incomplete word */
    if (xme == 1) {
        IFX_USIF_TXD_SIDEBAND_LAST_WORD_CONFIG(xbe, port);
    }
    IFX_USIF_PUT_TX_WORD(txd_data.txd_word, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
    stat->tx_write_fifo++;
    return 0;
}

/**
 * \fn     static void ifx_usif_uart_tx_check_stopped(struct uart_port *port)
 *
 * \brief  Helper function transmit interrupts, to check whether the 
 *         data transmit is stopped.
 *
 *         The data transmission could be forced to stop, due to flow control
 *         de-assertion by TTY layer.
 *
 * \param  port   pointer to USIF uart port.
 *            
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void
ifx_usif_uart_tx_check_stopped(struct uart_port *port)
{
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    if (uart_tx_stopped(port)) {
        stat->tx_stopped++;
        IFX_USIF_UART_IRQ_LOCK(port);
        IFX_USIF_CLR_INT_MASK(IFX_USIF_INT_TRANSMIT, port);
        IFX_USIF_UART_IRQ_UNLOCK(port);
    }
}

/**
 * \fn     static void ifx_usif_uart_transmit_chars(struct uart_port *port, unsigned int burst_type)
 *
 *
 * \brief  Transmit interrupt service routine.
 *
 *         The TOPSPIN Transmit FIFO is configured as Flow-Controller mode, 
 *         byte aligned and the burst size is set to four words. So each transmitted
 *         word will have four characters
 *
 * \param  port        pointer to USIF uart port.
 * \param  burst_type  Interrupt source, burst type.
 *            
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void 
ifx_usif_uart_transmit_chars(struct uart_port *port, unsigned int burst_type)
{
    u32 word_cnt;
    u32 tx_fifo_fs;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    
    /* Get the TX FIFO filled stages */
    tx_fifo_fs = IFX_USIF_GET_TX_FILL_FIFO(port);

    /* 
     * Service the transmit request, depending on burst type 
     * XXX, should decode every bit, but USIF BREQ/SREQ will come together 
     * if TX FIFO is empty. 
     */
    switch (burst_type) {
        case IFX_USIF_INT_TX_REQ:
        case IFX_USIF_INT_TX_BREQ:
            /* Check whether sufficient empty space in FIFO */
            if ((IFX_USIF_GET_TX_FIFO_SIZE(port) - tx_fifo_fs) < IFX_USIF_UART_TX_BUSRT_SIZE) {
                stat->tx_req_fifo_full++;
            }
            else {
                for (word_cnt = 0; word_cnt < IFX_USIF_UART_TX_BUSRT_SIZE; word_cnt++) {
                    if (!ifx_usif_uart_put_tx_word(port)) {
                        break;
                    }
                }
            }
   
            /*
             * The transmission cannot be stopped while the data is partially written during
             * the burst request. Note that, the data is transmitted in 4 words (4*4 characters)
             * per burst request and 4 characters per single request
             */
            ifx_usif_uart_tx_check_stopped(port);
            stat->tx_intr_breq++;
            IFX_USIF_UART_IRQ_LOCK(port);
            IFX_USIF_CLR_INT(burst_type, port);
            IFX_USIF_UART_IRQ_UNLOCK(port);
            break;
 
        case IFX_USIF_INT_TX_SREQ: 
            /* Check whether sufficient empty space in FIFO */
            if ((IFX_USIF_GET_TX_FIFO_SIZE(port) - tx_fifo_fs) < 0x01) {
                stat->tx_req_fifo_full++;
            }
            else {
                ifx_usif_uart_put_tx_word(port);
            }
            ifx_usif_uart_tx_check_stopped(port);
            stat->tx_intr_sreq++;
            IFX_USIF_UART_IRQ_LOCK(port);
            IFX_USIF_CLR_INT(burst_type, port);
            IFX_USIF_UART_IRQ_UNLOCK(port);
            break;
            
        default:
            printk(KERN_ERR "%s: Should not come here\n", __func__);
            break;
    }
}

/**
 * \fn     static irqreturn_t ifx_usif_uart_err_intr(int irq, void *dev_id)
 *
 * \brief  Error handler routine.
 *
 *         The IFX_USIF_INT_RXOF, IFX_USIF_INT_PE and IFX_USIF_INT_FE
 *         errors are handled while servicing the receive interrupts, so these
 *         interrupts mask are disabled.
 *
 * \param  irq         interrupt irq number
 * \param  dev_id      cast to USIF uart port
 *            
 * \return  IRQ_HANDLED irq has been handled
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static irqreturn_t
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
ifx_usif_uart_err_intr(int irq, void *dev_id)
#else
ifx_usif_uart_err_intr(int irq, void *dev_id, struct pt_regs *regs)
#endif 
{
    u32 status;
    struct uart_port *port = dev_id;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    status = IFX_USIF_GET_INT_STATUS(port) & IFX_USIF_INT_ERR_MASK;
    if (status & IFX_USIF_INT_RXUR) {
        stat->err_int_rxur++;
        IFX_USIF_CLR_INT(IFX_USIF_INT_RXUR, port);
    }
 
    if (status & IFX_USIF_INT_TXUR) {
        stat->err_int_txur++;
        IFX_USIF_CLR_INT(IFX_USIF_INT_TXUR, port);
    }
 
    if (status & IFX_USIF_INT_TXOF) {
        stat->err_int_txof++;
        IFX_USIF_CLR_INT(IFX_USIF_INT_TXOF, port);
    }
 
    if (status & IFX_USIF_INT_PHE) {
        stat->err_int_phe++;
        IFX_USIF_CLR_INT(IFX_USIF_INT_PHE, port);
    }
 
    if (status & IFX_USIF_INT_CRC) {
        stat->err_int_crc++;
        IFX_USIF_CLR_INT(IFX_USIF_INT_CRC, port);
    }
    
    /* 
     * WAR, flush write, fix hardware timing issue. Make sure device clear operation
     * happens before ICU clear operation. Other ways to do are mmiowb() and etc
     */    
    IFX_USIF_GET_INT_STATUS(port);
    return IRQ_HANDLED;
}

/**
 * \fn     static void ifx_usif_uart_start_tx(struct uart_port *port)
 *
 * \brief  This function is used to initiate a transmision
 * 
 * \param  port     pointer to USIF uart port  
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_start_tx(struct uart_port *port)
{
    u32 cr;
    struct circ_buf *xmit = UART_PORT_TO_XMIT(port);
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    stat->tx_start_req++;

    /*
     * Do not the start the transmission, if the circular buffer has no data or
     * the data transmission is stopped.
     * But need to transmit software flow control data (not implemented now)
     */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
        ifx_usif_uart_stop_tx(port);
        return;
    } 

    /*
     * If a XON/XOFF character needs to be transmitted out, the
     * x_char field of the port is set by the serial core
     */
    if (port->x_char) {
        IFX_USIF_UART_IRQ_LOCK(port);
        IFX_USIF_PUT_TX_CHAR(port->x_char, port);
        IFX_USIF_UART_IRQ_UNLOCK(port);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }
    
    /*
     * Initiate the transmission, only if the transmission is not active already or stopped due to
     * flow control
     */
    if ((usif_port_p->tx_progress == IFX_USIF_UART_TX_PROGRESS_INACTIVE))  {
        /*
         * Initiate the TPS register to start the transmission
         * Note that, the transmit FIFO in TOPSPIN module is used in flow control mode 
         */
        if (uart_circ_chars_pending(xmit)) {
            usif_port_p->tx_progress = IFX_USIF_UART_TX_PROGRESS_ACTIVE;
        }
        else {
            return; /* No data to send, return immediately */
        }
    } 
    /*
     * Set the TX mask. We need to process the TX interrupts.
     * Enable mask, if not enabled already
     */
    IFX_USIF_UART_IRQ_LOCK(port);
    cr = IFX_USIF_GET_INT_MASK(port);
    if ((cr & IFX_USIF_INT_TRANSMIT) != IFX_USIF_INT_TRANSMIT) {
        cr |= IFX_USIF_INT_TRANSMIT | IFX_USIF_INT_TX_FIN;
        IFX_USIF_SET_INT_MASK(cr, port);
    }
    IFX_USIF_UART_IRQ_UNLOCK(port);
} 

/**
 * \fn     static void ifx_usif_uart_stop_tx(struct uart_port *port)
 *
 * \brief  This function is used to stop the transmision
 *
 * \param  port     pointer to USIF uart port 
 *
 * \return  None    
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_stop_tx(struct uart_port *port)
{ 
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    /* Remove the TX mask. We do not want to process TX interrupts any more now */
    stat->tx_stop_req++;

    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT_MASK(IFX_USIF_INT_TRANSMIT | IFX_USIF_INT_TX_FIN, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static irqreturn_t ifx_usif_uart_status_intr(int irq, void *dev_id)
 *
 * \brief  STA interrupt handler routine.
 * 
 * \param  irq         interrupt irq number
 * \param   dev_id     pointer to USIF uart port
 *            
 * \return IRQ_HANDLED  irq has been handled
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static irqreturn_t
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
ifx_usif_uart_status_intr(int irq, void *dev_id)
#else
ifx_usif_uart_status_intr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    struct uart_port *port = dev_id;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);
    struct circ_buf *xmit = UART_PORT_TO_XMIT(port);
    u32 status, modem_status;

    status = IFX_USIF_GET_INT_STATUS(port) & IFX_USIF_INT_STA_ALL;
#ifdef IFX_USIF_UART_DEBUG
    prom_printf("%s irq %d USIF_MIS 0x%08x\n", __func__, irq, IFX_USIF_GET_INT_STATUS(port));
    prom_printf("%s irq %d USIF_RIS 0x%08x\n", __func__, irq, IFX_USIF_GET_RAW_INT_STATUS(port));
    prom_printf("%s irq %d USIF_IMSC 0x%08x\n", __func__, irq, IFX_USIF_GET_INT_MASK(port));
#endif 
    /* XXX, handle spurious interrupts */    
    if ((status & IFX_USIF_INT_FCI)) {
        modem_status = IFX_USIF_GET_MODEM_STATUS(port);
  
        uart_handle_cts_change(port, modem_status & IFX_USIF_MSS_STAT_FCI);
  
        wake_up_interruptible(&UART_PORT_TO_MSR(port));
  
        IFX_USIF_CLR_INT(IFX_USIF_INT_FCI, port);
    }
    if ((status & IFX_USIF_INT_TX_FIN)) {
        if (uart_tx_stopped(port)) {
            ifx_usif_uart_stop_tx(port);
            usif_port_p->tx_progress = IFX_USIF_UART_TX_PROGRESS_INACTIVE;
        }
        else {
            /* XXX, not available now Initiate another transmission, 
             * if the circular buffer has still characters to send 
             */
            if (uart_circ_empty(xmit)) {
                ifx_usif_uart_stop_tx(port);
                usif_port_p->tx_progress = IFX_USIF_UART_TX_PROGRESS_INACTIVE;
                uart_write_wakeup(port);
            }
            /* We still have data, no need to trigger another transaction */
         }
         stat->tx_intr_txfin++;
         IFX_USIF_CLR_INT(IFX_USIF_INT_TX_FIN, port);
    }

    if (status) {
         /*
          * Do nothing, just clear all interrupt sources
          * For some reason we need to do this, though these interrupt sources mask is disabled
          */
         IFX_USIF_CLR_INT(IFX_USIF_INT_STA_CLR_ALL, port);
    }
    /*
     * We need to handle other modem signals RI, DSR and DCD also, if the chip supports it.
     * Right now, USIF supports only CTS and RTS.
     */
    /* 
     * WAR, flush write, fix hardware timing issue. Make sure device clear operation
     * happens before ICU clear operation. Other ways to do are mmiowb() and etc
     */
    IFX_USIF_GET_INT_STATUS(port);
    return IRQ_HANDLED;
}

/**
 * \fn     static irqreturn_t ifx_usif_uart_data_intr(int irq, void *dev_id)
 *
 * \brief  Interrupt handler routine for receive and transmit.
 * 
 * \param  irq         interrupt irq number
 * \param  dev_id      cast to USIF uart port
 *            
 * \return IRQ_HANDLED  irq has been handled
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static irqreturn_t
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
ifx_usif_uart_data_intr(int irq, void *dev_id)
#else
ifx_usif_uart_data_intr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    u32 status;
    struct uart_port *port = dev_id;

    status = IFX_USIF_GET_INT_STATUS(port) & IFX_USIF_INT_DATA_ALL;
#ifdef IFX_USIF_UART_DEBUG
    prom_printf("%s irq %d USIF_MIS 0x%08x\n", __func__, irq, IFX_USIF_GET_INT_STATUS(port));
    prom_printf("%s irq %d USIF_RIS 0x%08x\n", __func__, irq, IFX_USIF_GET_RAW_INT_STATUS(port));
    prom_printf("%s irq %d USIF_IMSC 0x%08x\n", __func__, irq, IFX_USIF_GET_INT_MASK(port));
#endif
    /* Handle the RX interrupts */
    if (status & IFX_USIF_INT_RECEIVE) {
        /* Note that these interrupts are cleared in the service routine */
        if (status & IFX_USIF_INT_RX_BREQ) {
            ifx_usif_uart_rx_breq(port);
        }

        if (status & IFX_USIF_INT_RX_LBREQ) {
            ifx_usif_uart_rx_lbreq(port);
        }

        if (status & IFX_USIF_INT_RX_SREQ) {
            ifx_usif_uart_rx_sreq(port);
        }

        if (status & IFX_USIF_INT_RX_LSREQ) {
            ifx_usif_uart_rx_lsreq(port);
        }
    }
    /* Handle the Tx interrupt */
    if (status & IFX_USIF_INT_TRANSMIT) {
        /* Note that these interrupts are cleared in the service routine */
        ifx_usif_uart_transmit_chars(port, (status & IFX_USIF_INT_TRANSMIT));
    }
    /* 
     * WAR, flush write, fix hardware timing issue. Make sure device clear operation
     * happens before ICU clear operation. Other ways to do are mmiowb() and etc
     */
    IFX_USIF_GET_INT_STATUS(port);
    return IRQ_HANDLED; 
}

/**
 * \fn     static unsigned int ifx_usif_uart_tx_empty(struct uart_port *port)
 *
 * \brief  This function is used to check whether TX FIFO is empty or not
 * 
 * \param  port     pointer to USIF uart port
 *            
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static unsigned int
ifx_usif_uart_tx_empty(struct uart_port *port)
{
    return (IFX_USIF_GET_TX_FILL_FIFO(port) == 0) ? TIOCSER_TEMT : 0;
}

/**
 * \fn     static void ifx_usif_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
 *
 * \brief  This function is used to set the modem signals
 * 
 * \param  port     pointer to USIF uart port
 * \param  mctrl       
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    u32 m_set = 0x00;
    u32 m_clr = 0x00;
 
    if (mctrl & TIOCM_RTS) {
        m_set = IFX_USIF_MSS_SET_FCO;
    }
    else {
        m_clr = IFX_USIF_MSS_CLR_FCO;
    }
 
    if (mctrl & TIOCM_DTR) {
        m_set |= IFX_USIF_MSS_SET_DTR;
    }
    else {
        m_clr |= IFX_USIF_MSS_CLR_DTR;
    }
 
    if (m_set) {
        IFX_USIF_SET_MODEM_STATUS(m_set, port);
    }
    
    if (m_clr) {
        IFX_USIF_CLR_MODEM_STATUS(m_clr, port);
    }
}

/**
 * \fn     static unsigned int ifx_usif_uart_get_mctrl(struct uart_port *port)
 *
 * \brief  This function is used to get the modem signal status
 * 
 * \param  port     pointer to USIF uart port   
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static unsigned int
ifx_usif_uart_get_mctrl(struct uart_port *port)
{
    u32 status;
    u32 ret = 0;

    status = IFX_USIF_GET_MODEM_STATUS(port);
    if (status & IFX_USIF_MSS_STAT_FCI) {
        ret = TIOCM_CTS;
    }
 
    if (status & IFX_USIF_MSS_STAT_DCD) {
        ret |= TIOCM_CAR;
    }
    
    if (status & IFX_USIF_MSS_STAT_RI) {
        ret |= TIOCM_RNG;
    }
    
    if (status & IFX_USIF_MSS_STAT_DSR) {
        ret |= TIOCM_DSR;
    }
 
    /* 
     * If the port does not support CTS, DCD (CAR) or DSR, the driver should indicate
     * that the signal is permanently active 
     */
    ret |= (TIOCM_DSR |TIOCM_CAR);
    return ret;
}

/**
 * \fn     static void ifx_usif_uart_stop_rx(struct uart_port *port)
 *
 * \brief  This function is used to stop reception
 * 
 * \param  port     pointer to USIF uart port  
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_stop_rx(struct uart_port *port)
{
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT_MASK(IFX_USIF_INT_RECEIVE, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static void ifx_usif_uart_enable_ms(struct uart_port *port)
 *
 * \brief  This function is used to enable the modem signal interrupts
 * 
 * \param  port     pointer to USIF uart port  
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_enable_ms(struct uart_port *port)
{
    /* Set the STA mask. We would like to process the CTS interrupt */
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_CLR_INT_MASK(IFX_USIF_INT_FCI, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
}

/**
 * \fn     static void ifx_usif_uart_break_ctl(struct uart_port *port, int break_state)
 *
 * \brief  This function is used to send break signal
 * 
 * \param  port        pointer to USIF uart port  
 * \param  break_state break state
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_break_ctl(struct uart_port *port, int break_state)
{
    /* The USIF module does not support this now */
}

/**
 * \fn static INLINE void ifx_usif_uart_clc_cfg_mode(struct uart_port *port)
 * \brief enter configuration mode.
 *
 * \param   port    pointer to USIF uart port 
 * 
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static INLINE void
ifx_usif_uart_clc_cfg_mode(struct uart_port *port)
{
    int i;
#define IFX_USIF_UART_CLC_CFG_TIMEOUT  10 

    /* 
     * HW WAR, dummy read to make sure the previous read/write to take effect
     * before we go to config mode
     * Please note, must be a config register, instead of status register
     */
#ifndef CONFIG_USE_EMULATOR
    IFX_USIF_GET_MODE_CONFIG(port);
#endif /* CONFIG_USE_EMULATOR */
    IFX_USIF_CLC_CONFIG_MODE(port);
    for (i = 0; i < IFX_USIF_UART_CLC_CFG_TIMEOUT; i++) {
        if ((IFX_USIG_GET_CLOCK_STATUS(port) & IFX_USIF_CLC_STAT_CFG_UPDATE_DONE) 
            == IFX_USIF_CLC_STAT_CFG_UPDATE_DONE) {
            break;
        }
    }
    if (i >= IFX_USIF_UART_CLC_CFG_TIMEOUT) {
        printk(KERN_ERR "%s timeout\n", __func__);
    }
#undef IFX_USIF_UART_CLC_CFG_TIMEOUT
}

/**
 * \fn static INLINE void ifx_usif_uart_clc_run_mode(struct uart_port *port)
 * \brief enter running mode.
 *
 * \param   port    pointer to USIF uart port  
 * 
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static INLINE void
ifx_usif_uart_clc_run_mode(struct uart_port *port)
{
    int i;
#define IFX_USIF_UART_CLC_RUN_TIMEOUT  10
    /* 
     * WAR, Dummy read to make sure the previous configuration to take effect 
     * before we go to running mode.
     * Please note, must be a config register, instead of status register
     */
#ifndef CONFIG_USE_EMULATOR
    IFX_USIF_GET_MODE_CONFIG(port);
#endif /* CONFIG_USE_EMULATOR */
    IFX_USIF_CLC_RUN_MODE(port);
    for (i = 0; i < IFX_USIF_UART_CLC_RUN_TIMEOUT; i++) {
        if ((IFX_USIG_GET_CLOCK_STATUS(port) & IFX_USIF_CLC_STAT_RUN_UPDATE_DONE) 
            == IFX_USIF_CLC_STAT_RUN_UPDATE_DONE) {
            break;
        }
    }
    if (i >= IFX_USIF_UART_CLC_RUN_TIMEOUT) {
        printk(KERN_ERR "%s timeout\n", __func__);
    }
#undef IFX_USIF_UART_CLC_RUN_TIMEOUT
}

/**
 * \fn     static int ifx_usif_uart_startup(struct uart_port *port)
 *
 * \brief  This function is called while starting the serial port
 * 
 *         Initialise all interrupts and assign priority to each of them.
 *         But enable only the receive interupts
 * 
 * \param  port        pointer to USIF uart port  
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static int
ifx_usif_uart_startup(struct uart_port *port)
{
    int ret = 0;
    u32 reg;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    struct tty_struct *tty =  UART_PORT_TO_TTY(port);
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    IFX_USIF_UART_PRINT(usif_port_p, USIF_UART_MSG_INIT, 
        "%s: port=%p (%08lx,%p)\n", __func__, port->mapbase, port->membase);

    IFX_USIF_UART_IRQ_LOCK(port);
    /* Configuration Mode */
    ifx_usif_uart_clc_cfg_mode(port); 
    /* Enable RX */  
    reg = IFX_USIF_GET_MODE_CONFIG(port);
    reg |= IFX_USIF_MODE_CFG_RX_EN;
    IFX_USIF_SET_MODE_CONFIG(reg, port);

    /* Enable TX flow control  */
    reg = IFX_USIF_GET_FIFO_CONFIG(port);
    reg |= IFX_USIF_FIFO_CFG_TXFC;
    IFX_USIF_SET_FIFO_CONFIG(reg, port);

    /* Switch to USIF working mode */
    ifx_usif_uart_clc_run_mode(port);

    /* XXX, Running time configuration */
    ifx_usif_uart_run_time_config(port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
    
    ret = request_irq(usif_port_p->tx_irq, ifx_usif_uart_data_intr, IRQF_SHARED, "USIF_TX_REQ", port);
    if (ret) {
        printk(KERN_ERR "%s: can't get irq %d for Tx\n", __func__, usif_port_p->tx_irq);
        goto done;
    }

    ret = request_irq(usif_port_p->rx_irq, ifx_usif_uart_data_intr, IRQF_SHARED, "USIF_RX_REQ", port);
    if (ret) {
        printk(KERN_ERR "%s: can't get irq %d for Rx\n", __func__, usif_port_p->rx_irq);
        goto err1;
    }
    
    /* Request for handle for STA interrupts, which is used to detect CTS status change */
    ret = request_irq(usif_port_p->status_irq, ifx_usif_uart_status_intr, IRQF_SHARED, "USIF_STAT", port);
    if (ret){
        printk(KERN_ERR "%s: can't get irq %d for status\n", __func__, usif_port_p->status_irq);
        goto err2;
    }
 
    /*
     * Request for handle for ERR interrupts
     * We may continue the normal operation, though unable to get handler here
     */
    ret = request_irq(usif_port_p->err_irq, ifx_usif_uart_err_intr, IRQF_SHARED, "USIF_ERR", port);
    if (ret)  {
        printk(KERN_ERR "%s: can't get irq %d for error\n", __func__, usif_port_p->err_irq);
        goto err3;
    }
    
    /* XXX, autobaud and wakeup interrupt */
  
    /* Reset the Rx and Tx local counters */
    usif_port_p->tx_progress = IFX_USIF_UART_TX_PROGRESS_INACTIVE;
    usif_port_p->rx_rps_cnt = 0x00;

    /*
     *  Update the 'low latency' flag before enabling the receive interrupts.
     *  Enable the low latency while pushing the received data into FLIP
     *  buffer. It is required only for higher baudrate operation or large
     *  data packets, where each data packet size is more than 512 bytes.
     *  Enabling low latency could consume more CPU while servicing
     *  receive interrupts. If the low latency flag is not enabled, then
     *  there are chances of loosing received data from RX FIFO.
     *  Note: It is advised to use hardware flowcontrol while using higher
     *  baudrates and large data packets, as it is observed that, the data
     *  has been dropped without this
     */
    tty->low_latency = ((port->flags & ASYNC_LOW_LATENCY) ? 1 : 0);
 
    stat->rx_low_latency = tty->low_latency;

    IFX_USIF_UART_IRQ_LOCK(port);
    /* Enable only the receive interrupts, TX interrupt will be enabled later */
    IFX_USIF_CLR_INT(IFX_USIF_INT_CLR_ALL, port);
    IFX_USIF_SET_INT_MASK(IFX_USIF_INT_RECEIVE, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);
#ifdef IFX_USIF_UART_DEBUG
    ifx_usif_uart_reg_dump(port);
#endif
    IFX_USIF_UART_PRINT(usif_port_p, USIF_UART_MSG_INIT, 
        "%s succeed\n", __func__);
    return 0;
err3:
    free_irq(usif_port_p->status_irq, port);
err2:
    free_irq(usif_port_p->rx_irq, port);
err1:
    free_irq(usif_port_p->tx_irq, port);
done:
    return ret;
}

/**
 * \fn     static void ifx_usif_uart_shutdown(struct uart_port *port)
 *
 * \brief  This function is called while closing the serial port
 * 
 *         De-initialise all interrupts and assign prioroty to each of them.
 * 
 * \param  port        pointer to USIF uart port  
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_shutdown(struct uart_port *port)
{
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;

    /* Switch to USIF configuration mode  */
    ifx_usif_uart_clc_cfg_mode(port);

    /* Disable all interrupts, disable the port */
    IFX_USIF_CLR_INT(IFX_USIF_INT_CLR_ALL, port);
    IFX_USIF_SET_INT_MASK(0, port);
  
    /* Free the interrupt */
    free_irq(usif_port_p->tx_irq, port);         /* TX interrupt */
    free_irq(usif_port_p->rx_irq, port);         /* RX interrupt */
    free_irq(usif_port_p->status_irq, port);     /* STATUS interrupt */
    free_irq(usif_port_p->err_irq, port);        /* ERROR interrupt */
}

/**
 * \fn static unsigned int  ifx_usif_uart_get_kernel_clk(struct uart_port *port)
 * \brief Get USIF UART clock speed.
 * Returns the current operating speed of the UART, depending on chip
 * specific bus speed and RMC setting in CLC register.
 * 
 * \param   port     Pointer to structure #struct uart_port
 * \return  >0       Peripheral speed in HZ
 * \return  0        Error
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static unsigned int 
ifx_usif_uart_get_kernel_clk(struct uart_port *port)
{
    /* This function assumes that the CLC register is set with the
     * appropriate value for RMC.
     */
    u32 rmc;

    rmc = MS(IFX_USIF_GET_CLOCK_COUNTER(port), IFX_USIF_CLC_CNT_RMC);
    if (rmc == 0) {
        printk(KERN_ERR "%s rmc==0 \n", __func__);
        return 0;
    }
    return (unsigned int)(ifx_get_usif_hz() / rmc);
}

/**
 * \fn static unsigned int ifx_usif_uart_clk_to_baudtbl_idx(struct uart_port * port)
 * \brief convert usif kernel clock to baud table index
 * 
 * \param   port     Pointer to structure #struct uart_port
 * \return  int      baud table index 0~3
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static int 
ifx_usif_uart_clk_to_baudtbl_idx(struct uart_port * port)
{
    int baud_tbl_idx;
    
    unsigned int clk = ifx_usif_uart_get_kernel_clk(port);

    switch (clk) {
        case 78583:
            baud_tbl_idx = 0;
            break;
        case 1000000:
            baud_tbl_idx = 1;
            break;
        case 100000000:
            baud_tbl_idx = 2;
            break;
        case 20000000:
            baud_tbl_idx = 3;
            break;
        default:
            baud_tbl_idx = 2;
            break;
    }
    return baud_tbl_idx;
}

/**
 * \fn     static u32 ifx_usif_uart_get_baud(struct uart_port *port)
 *
 * \brief  This function is used to obtain baudrate according to
 *         fdiv and baud counter register configuration
 *
 * \param  port      pointer to usif uart port.
 *
 * \return None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
#ifdef CONFIG_IFX_USIF_UART_CONSOLE
static u32 
ifx_usif_uart_get_baud(struct uart_port *port)
{
    u32 inc, dec;
    u32 bcrv;
    u32 fdiv;
    u32 ibaud = IFX_USIF_UART_DEFAULT_BAUDRATE;

    fdiv = IFX_USIF_GET_FRAC_DIV(port);
    
    inc = MS(fdiv, IFX_USIF_FDIV_CFG_INC);
    dec = MS(fdiv, IFX_USIF_FDIV_CFG_DEC);

    bcrv = MS(IFX_USIF_GET_BAUDRATE_COUNTER(port), IFX_USIF_BC_CFG_BCRV);
    bcrv += 1;
    
    if (inc == 0 || dec == 0 ) { /* fdiv disabled */
        ibaud = (u32)((port->uartclk >> 4)/ bcrv);
    }

    if (inc != 0 && dec != 0) {
        /* XXX, possible overflow */
        ibaud = (u32)((port->uartclk >> 4) * inc / ((inc + dec) * bcrv));
    }
    return ibaud;
}
#endif /* CONFIG_IFX_USIF_UART_CONSOLE */

/**
 * \fn     static IFX_USIF_HW_BAUDRATE_IDX_t ifx_usif_uart_baud_to_idx(unsigned int baud)
 *
 * \brief  This function is used to convert baud rate to baud rate index
 *  
 * \param  baud        baud rate to be converted
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static IFX_USIF_HW_BAUDRATE_IDX_t 
ifx_usif_uart_baud_to_idx(unsigned int baud)
{
    IFX_USIF_HW_BAUDRATE_IDX_t baudrate = IFX_USIF_HW_BPS_115200;
    
    switch (baud) {
        case 300:
            baudrate = IFX_USIF_HW_BPS_300;
            break;
        case 1200:
            baudrate = IFX_USIF_HW_BPS_1200;
            break;
        case 2400:
            baudrate = IFX_USIF_HW_BPS_2400;
            break;
        case 4800:
            baudrate = IFX_USIF_HW_BPS_4800;
            break;
        case 9600:
            baudrate = IFX_USIF_HW_BPS_9600;
            break;
        case 19200:
            baudrate = IFX_USIF_HW_BPS_19200;
            break;
        case 38400:
            baudrate = IFX_USIF_HW_BPS_38400;
            break;
        case 57600:
            baudrate = IFX_USIF_HW_BPS_57600;
            break;
        case 115200:
            baudrate = IFX_USIF_HW_BPS_115200;
            break;
        case 0:

        default:
            /* Set the default baudrate as 115200 */
            break;
    }
    return baudrate;
}

/**
 * \fn     static void ifx_usif_uart_baud_setup(struct uart_port *port, u32 baud)
 *
 * \brief  This function is used to set up uart baudrate
 *  
 * \param  port        pointer to USIF uart port  
 * \param  baud        baud rate to be set up
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void 
ifx_usif_uart_baud_setup(struct uart_port *port, u32 baud)
{
    int baud_tbl_index;
    u32 baud_bc_cfg;
    u32 ictmo_cfg;
    IFX_USIF_HW_BAUDRATE_IDX_t baudrate;
    IFX_USIF_BAUDRATE_FDIV_t baud_fdiv;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;

    if (usif_port_p->prev_baud == baud) {
        return;
    }

    usif_port_p->prev_baud = baud;
    
    /* Set the default baud rate configuration, as 115200 */
    baudrate = ifx_usif_uart_baud_to_idx(baud);
    baud_tbl_index = ifx_usif_uart_clk_to_baudtbl_idx(port);

    baud_fdiv.fdiv_cfg_bits.inc = ifx_usif_uart_baudrate_table[baud_tbl_index].fpi[baudrate].inc;
    baud_fdiv.fdiv_cfg_bits.dec = ifx_usif_uart_baudrate_table[baud_tbl_index].fpi[baudrate].dec;
    baud_fdiv.fdiv_cfg_bits.dummy = 0;
    baud_bc_cfg = ifx_usif_uart_baudrate_table[baud_tbl_index].fpi[baudrate].bc;
    ictmo_cfg = ifx_usif_uart_baudrate_table[baud_tbl_index].fpi[baudrate].tmo;

    /* fdiv < inc, dec >, bc <bcriv> */
    IFX_USIF_FRAC_DIV_CONFIG(baud_fdiv.fdiv_cfg_word, port);
 
    /* Update the baudrate counter configuration register for baudrate */
    IFX_USIF_BAUDRATE_COUNTER_CONFIG(baud_bc_cfg, port);

    /*
     * Update the intercharacter timeout configuration register, for RX
     * Value not calculated/optinimed, just set some large value
     * Observed that, this value is small or not set, then RPS is not updated
     */
    IFX_USIF_INTERCHARACTER_TIMEOUT_CONFIG(ictmo_cfg, port);
}

/**
 * \fn     static void ifx_usif_uart_set_termios(struct uart_port *port, struct ktermios *termios,
 *            struct ktermios *old)
 *
 * \brief  This function is used to update the serial hardware setup,
 *         due to change in teminal I/O settings.
 *  
 *         Update all the USIF hardware setup depending on the terminal I/O setup 
 * 
 * \param  port        pointer to USIF uart port  
 * \param  termios     structure pointer to present terminal I/O settngs
 * \param  old         structure pointer to old terminal I/O settngs
 *
 * \return  None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */

static void
ifx_usif_uart_set_termios(struct uart_port *port, struct ktermios *termios,
             struct ktermios *old)
{
    u32 old_int_mask;
    u32 mode_cfg;
    u32 prtc_cfg; 
    u32 baud;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    prtc_cfg = IFX_USIF_GET_PROTOCOL_CONFIG(port);
    prtc_cfg &= ~IFX_USIF_PRTC_CFG_CLEN;
    switch (termios->c_cflag & CSIZE) {
        case CS5:
            prtc_cfg |= IFX_USIF_PRTC_CFG_CLEN5;
            break;
 
        case CS6:
            prtc_cfg |= IFX_USIF_PRTC_CFG_CLEN6;
            break;
 
        case CS7:
            prtc_cfg |= IFX_USIF_PRTC_CFG_CLEN7;
            break;
 
        case CS8:
        default:
            prtc_cfg |= IFX_USIF_PRTC_CFG_CLEN8;
            break;
    }    
    if (termios->c_cflag & CSTOPB) {
        /* Use two stop bits */
        prtc_cfg |= IFX_USIF_PRTC_CFG_STP2;
    }
 
    if (termios->c_cflag & PARENB) {
        /* Enable parity, by default even parity */
        prtc_cfg |= IFX_USIF_PRTC_CFG_PAR;
  
        /* Set odd parity */
        if (termios->c_cflag & PARODD) {
            prtc_cfg |= IFX_USIF_PRTC_CFG_ODD;
        }
    }

    mode_cfg = IFX_USIF_GET_MODE_CONFIG(port);
    
    /* Ignore all received characters if CREAD is not set */
    if ((termios->c_cflag & CREAD) == 0) {
        mode_cfg &= ~IFX_USIF_MODE_CFG_RX_EN;
    }
    else {
        mode_cfg |= IFX_USIF_MODE_CFG_RX_EN;
    }
  
    /* 
     * This driver does support the hardware flowcontrol, which is in-turn
     * controlled by software, not by the internal hardware of USIF module 
     */
    if (termios->c_cflag & CRTSCTS) {
        prtc_cfg |= (IFX_USIF_PRTC_CFG_SFCI_EN | IFX_USIF_PRTC_CFG_SFCO_EN);
        prtc_cfg |= SM(8, IFX_USIF_PRTC_CFG_FCO_TL); /* XXX, check trigger level */
    }

    /* Ask the core to calculate the divisor for us */
    baud = uart_get_baud_rate(port, termios, old, 0, IFX_USIF_UART_MAX_BAUDRATE); 

    /* Wait till the FIFO becomes empty before we enter the configure mode */
    while (IFX_USIF_GET_TX_FILL_FIFO(port) != 0x00){
        ;
    }
        
    /* Sanity Check again */
    if (((IFX_USIF_GET_FIFO_STATUS(port) & IFX_USIF_FIFO_STAT_TXRX_FFS_MASK) != 0x00)){
        printk(KERN_ERR "%s busy\n", __func__);
        return;
    }

    IFX_USIF_UART_IRQ_LOCK(port);
 
    /* Update the per-port timeout */
    uart_update_timeout(port, termios->c_cflag, baud);
 
    /* Update the read_status_mask and ignore_status_mask */
 
    /* RX overflow/overrun error */
    port->read_status_mask = IFX_USIF_INT_RXOF;
 
    if (termios->c_iflag & INPCK) {
        port->read_status_mask |= IFX_USIF_INT_FE | IFX_USIF_INT_PE;
    }

    if (termios->c_iflag & (BRKINT | PARMRK)) {
        /* USIF does not support the break interrupt error */
        port->read_status_mask |= port->read_status_mask;
    }
 
    /* Characters to ignore */
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR) {
        port->ignore_status_mask |= IFX_USIF_INT_FE | IFX_USIF_INT_PE;
    }
 
    if (termios->c_iflag & IGNBRK) {
        /*
         * If we're ignoring parity and break indicators,
         *   ignore overruns too (for real raw support)
         */
        if (termios->c_iflag & IGNPAR) {
            port->ignore_status_mask |= IFX_USIF_INT_RXOF;
        }
    }
 
    old_int_mask = IFX_USIF_GET_INT_MASK(port);
    /* Switch to USIF configuration mode */
    ifx_usif_uart_clc_cfg_mode(port);    

    /* Disable all interrupts */
    IFX_USIF_SET_INT_MASK(0, port);

    if (usif_port_p->prev_mode_cfg != mode_cfg) {
        /* Update the mode configuration register with the settings */
        IFX_USIF_SET_MODE_CONFIG(mode_cfg, port);
        usif_port_p->prev_mode_cfg = mode_cfg;
    }

    if (usif_port_p->prev_prtc_cfg != prtc_cfg) {
        /* Update the protocol configuration register with the settings */
        IFX_USIF_SET_PROTOCOL_CONFIG(prtc_cfg, port);
        usif_port_p->prev_prtc_cfg = prtc_cfg;
    }

    ifx_usif_uart_baud_setup(port, baud);
    
    /* Enable the CTS interrupt  */
    if (termios->c_cflag & CRTSCTS) {
        /* We care about only CTS interrupt, now  */
        old_int_mask |= IFX_USIF_INT_FCI;
    }
    else {
        /* Disable the CTS interrupt */
        old_int_mask &= (~IFX_USIF_INT_FCI);
    }
 
    /* We need to take care the transmission finished interrupt  */
    old_int_mask |= IFX_USIF_INT_TX_FIN;
 
    /* Set the mask, to enable STA/ ERR interrupts */
    old_int_mask |= IFX_USIF_INT_ERR_MASK | IFX_USIF_INT_STA_ALL;

    /* XXX, in release, TMO interrupt doesn't need */
    old_int_mask &= ~IFX_USIF_INT_TMO;

    /* Switch to USIF working mode */
    ifx_usif_uart_clc_run_mode(port);

    ifx_usif_uart_run_time_config(port);
    
    /* Set the mask for interrupts */
    IFX_USIF_CLR_INT(IFX_USIF_INT_CLR_ALL, port);
    IFX_USIF_SET_INT_MASK(old_int_mask, port);
    IFX_USIF_UART_IRQ_UNLOCK(port);

    /* To make baudrate configuration take effect, we need some certain delay */
    udelay(10);
    
    stat->tx_bytes = port->icount.tx;
    stat->rx_bytes = port->icount.rx;

}

/**
 * \fn     static const char* ifx_usif_uart_type(struct uart_port *port)
 *
 * \brief  This function is used to get the pointer to a string 
 *         constant describing the port  
 *
 * \param  port        pointer to USIF uart port  
 *
 * \return  "USIF_UART"
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static const char*
ifx_usif_uart_type(struct uart_port *port)
{
    return (port->type == IFX_PORT_USIF_UART) ? "USIF_UART" : NULL;
}

/** 
 *
 * \fn    static void ifx_usif_uart_release_port(struct uart_port *port)
 *
 * \brief This function is used to release the memory used by the port
 *
 * \param  port - pointer to USIF uart port.
 *
 * \return NONE
 *
 * NOTES: Releases the virtual memory allocated to USIF using 
 *        'ifx_usif_uart_request_port' function.
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_release_port(struct uart_port *port)
{
    iounmap(port->membase);
}

/**
 * \fn     static int ifx_usif_uart_request_port(struct uart_port *port)
 *
 * \brief  This function is used to allocate the memory for the port 
 *
 *          Allocates virtual memory to USIF using the physical address pointed
 *         'mapbase' using the 'ioremap_nocache' utility
 *
 * \param  port        pointer to USIF uart port 
 *
 * \return 0           successfully
 *         -EBUSY
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static int
ifx_usif_uart_request_port(struct uart_port *port)
{
    IFX_USIF_UART_PORT_t *usif_port_p;
    
    usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    port->membase = ioremap_nocache(port->mapbase, IFX_USIF_SIZE);
    if (!port->membase) {
        /* If USIF is used as console and failed here, restore the hardcoded virtual address  */
#ifdef CONFIG_IFX_USIF_UART_CONSOLE
        port->membase = (char *)(IFX_USIF_VIRT_BASE);
#endif
        printk(KERN_ERR "%s: Failed during io remap\n", __func__);
        return -EBUSY;
    }
    /* XXX, enter running mode? */
    IFX_USIF_UART_PRINT(usif_port_p, USIF_UART_MSG_INIT, 
        "%s: io remap 0x%x\n", __func__, (unsigned int) port->membase);
    return 0;
}

/**
 * \fn     static void ifx_usif_uart_config_port(struct uart_port *port, int flags)
 *
 * \brief  This function is used to perform any autoconfiguration steps
 *         required for the port.
 *
 *         Just calls 'ifx_usif_uart_request_port' function, if the port type is 
 *         'UART_CONFIG_TYPE'
 *
 * \param  port        pointer to USIF uart port 
 * \param  flags       Indicate type of port
 *
 * \return None           
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_config_port(struct uart_port *port, int flags)
{
    if (flags & UART_CONFIG_TYPE) {
        if (!ifx_usif_uart_request_port(port)) {
            port->type = IFX_PORT_USIF_UART;
        }
    }
}

/**
 * \fn     static int ifx_usif_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
 *
 * \brief  This function is used to verify the new serial port information 
 *         contained within serial information is suitable for this port type
 *
 * \param  port      pointer to USIF uart port 
 * \param  ser       serial_struct, details about the port
 *
 * \return None           
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static int
ifx_usif_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    int ret = 0;

    if ((ser->type != PORT_UNKNOWN) && (ser->type != IFX_PORT_USIF_UART)) {
        ret = -EINVAL;
    }
    
    if ((ser->irq < 0) || (ser->irq >= NR_IRQS)) {
        ret = -EINVAL;
    }
    
    if ((ser->baud_base) < (IFX_USIF_BAUD_MIN)) {
        ret = -EINVAL;
    }
    
    return ret;
}

/*
 * This structure has the callback functions used by the higher layers of
 * TTY driver. Initialise it with appropriate functions implmented.
 */
static struct uart_ops ifx_usif_uart_ops = {
    .tx_empty        = ifx_usif_uart_tx_empty,
    .set_mctrl       = ifx_usif_uart_set_mctrl,
    .get_mctrl       = ifx_usif_uart_get_mctrl,
    .stop_tx         = ifx_usif_uart_stop_tx,
    .start_tx        = ifx_usif_uart_start_tx,
    .stop_rx         = ifx_usif_uart_stop_rx,
    .enable_ms       = ifx_usif_uart_enable_ms,
    .break_ctl       = ifx_usif_uart_break_ctl,
    .startup         = ifx_usif_uart_startup,
    .shutdown        = ifx_usif_uart_shutdown,
    .set_termios     = ifx_usif_uart_set_termios,
    .type            = ifx_usif_uart_type,
    .release_port    = ifx_usif_uart_release_port,
    .request_port    = ifx_usif_uart_request_port,
    .config_port     = ifx_usif_uart_config_port,
    .verify_port     = ifx_usif_uart_verify_port,
};

/*
 * This structure is the main enrty point to this driver. Initialise it with the
 *   basic information about the device.
 *   Note:
 *   1. The 'membase' is initialized later, with the ACTUAL virtual address
 *       using 'ioremap_nocache' utility.
 *   2. The console setup uses the hardcoded virtual address, to print the
 *       console messages during startup, but once this driver is up, it uses
 *       the virtual address generated by 'ioremap_nocache' utility.
 */
static IFX_USIF_UART_PORT_t ifx_usif_uart_ports[IFX_USIF_UART_NR] = {
    {
        .port = {
              .iobase     = (unsigned int)IFX_USIF_VIRT_BASE,
              .membase    = (unsigned char __iomem *)(IFX_USIF_VIRT_BASE),/* Virtual address */
              .mapbase    = IFX_USIF_PHY_BASE, /* Physical address */
              .iotype     = SERIAL_IO_MEM,
              .irq        = IFX_USIF_RX_INT,   /* XXX, */
              .uartclk    = 0,  /* Dynamically */
              .fifosize   = IFX_USIF_UART_TX_FIFO_LENGTH,
              .type       = IFX_PORT_USIF_UART,
              .ops        = &ifx_usif_uart_ops,
        #ifdef CONFIG_USIF_UART_LOW_LATENCY
              .flags      = (ASYNC_BOOT_AUTOCONF | ASYNC_LOW_LATENCY),
        #else
              .flags      = ASYNC_BOOT_AUTOCONF,
        #endif
              .line       = 0,  /* Port index in one uart driver */
        },
        .tx_progress = IFX_USIF_UART_TX_PROGRESS_INACTIVE,
        .rx_rps_cnt  = 0,
        .tx_irq      = IFX_USIF_TX_INT,
        .rx_irq      = IFX_USIF_RX_INT,
        .wakeup_irq  = IFX_USIF_WKP_INT,
        .autobaud_irq= IFX_USIF_AB_INT,
        .status_irq  = IFX_USIF_STA_INT,
        .err_irq     = IFX_USIF_EIR_INT,
        .debug       = USIF_UART_MSG_ANY,
    }
};

/**
 * \fn static void ifx_usif_uart_gpio_init(void)
 * \brief Reserve and initialize GPIO for USIF/UART.
 *
 * Initialize MUX settings to enable USIF/UART interface
 * \ingroup IFX_USIF_UART_INTERNAL  
 */
static void 
ifx_usif_uart_gpio_init(void)
{
    ifx_gpio_deregister(IFX_GPIO_MODULE_USIF_UART);
    ifx_gpio_register(IFX_GPIO_MODULE_USIF_UART);
}

/**
 * \fn static void usif_uart_gpio_release(void)
 * \brief USIF/UART GPIO release.
 *
 * Release reserverd gpio resource so that other module could use it
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static void 
usif_uart_gpio_release(void)
{
    ifx_gpio_deregister(IFX_GPIO_MODULE_USIF_UART);
}

/**
 * \fn static inline void ifx_usif_uart_pm_enable(void)
 * \brief Clock gating on USIF module
 *
 * \return none
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static inline void
ifx_usif_uart_pm_enable(void)
{
    USIF_PMU_SETUP(IFX_PMU_ENABLE);
    AHBM_PMU_SETUP(IFX_PMU_ENABLE);
}

/**
 * \fn static inline void ifx_usif_uart_pm_disable(void) 
 * \brief Clock gating off USIF module
 *
 * \return none
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static inline void
ifx_usif_uart_pm_disable(void)
{
    USIF_PMU_SETUP(IFX_PMU_DISABLE);
    AHBM_PMU_SETUP(IFX_PMU_DISABLE);
}

/**
 * \fn     static int ifx_usif_uart_hw_init(void)
 *
 * \brief  This function is used to initialize USIF hardware configuration
 *
 * \return 0       OK
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static int 
ifx_usif_uart_hw_init(void)
{
    u32 reg =  0;
    struct uart_port *port;
    IFX_USIF_UART_PORT_t *usif_port_p;
    
    port = &ifx_usif_uart_ports[0].port;

    usif_port_p = (IFX_USIF_UART_PORT_t *)port;

    /* GPIO initialization */
    ifx_usif_uart_gpio_init();

    /* Enable USIF/AHBS */
    ifx_usif_uart_pm_enable();

     /* RMC first for AHB clock dmain  */
    IFX_USIF_SET_CLOCK_COUNTER((SM(IFX_USIF_UART_DEFAULT_RMC, IFX_USIF_CLC_CNT_RMC) |
        SM(IFX_USIF_UART_DEFAULT_ORMC, IFX_USIF_CLC_CNT_ORMC)), port);
     
    /* Switch to USIF configuration mode */
    reg = IFX_USIF_CLC_MOD_EN;
    IFX_USIF_SET_RUN_CONTROL(reg, port);
    ifx_usif_uart_clc_cfg_mode(port);
    
    /* Disable all interrupts */
    IFX_USIF_SET_INT_MASK(0, port);
    IFX_USIF_CLR_INT(IFX_USIF_INT_ALL, port);

    /* Mode configuration, Master, async, Enable Tx only 
     * NB, console only needs TX, serial driver startup will
     * Enable RX.
     */
    reg = IFX_USIF_MODE_CFG_TX_EN | IFX_USIF_MODE_CFG_MA;
    IFX_USIF_SET_MODE_CONFIG(reg, port);
 
    /* One byte per character, LSB first, even parity */
    reg = IFX_USIF_PRTC_CFG_CLEN8 /* | IFX_USIF_PRTC_CFG_ERR_FE | IFX_USIF_PRTC_CFG_PAR*/;
    IFX_USIF_SET_PROTOCOL_CONFIG(reg, port);
    
    /* Baudrate Setup */
    ifx_usif_uart_baud_setup(port, IFX_USIF_UART_DEFAULT_BAUDRATE);
    
    /* Configure the TX/RX FIFO */
    reg = IFX_USIF_UART_FIFO_SETUP;
    /* Enable TX swap and RX swap  */
    reg |= IFX_USIF_FIFO_CFG_TX_SWAP | IFX_USIF_FIFO_CFG_RX_SWAP;
    IFX_USIF_SET_FIFO_CONFIG(reg, port);

    /* IMP, must be set after FIFO_CFG during configuraiton phase */
    IFX_USIF_SET_FIFO_CTRL(IFX_USIF_FIFO_CTRL_RX_AR_ON, port);

    /* Switch to USIF working mode */
    ifx_usif_uart_clc_run_mode(port);

    ifx_usif_uart_run_time_config(port);
    
    port->uartclk = ifx_usif_uart_get_kernel_clk(port);
    IFX_USIF_UART_PRINT(usif_port_p, USIF_UART_MSG_INIT, "%s: uart kernel clock %d\n", __func__, port->uartclk);
    ifx_usif_uart_hw_initialized = 1;
#ifdef IFX_USIF_UART_DEBUG
    ifx_usif_uart_reg_dump(port);
#endif /* IFX_USIF_UART_DEBUG */
    return 0;
}

#ifdef CONFIG_IFX_USIF_UART_CONSOLE

/**
 * \fn     static void ifx_usif_uart_outb(struct uart_port *port, const char c)
 *
 * \brief  This function is used to write one character
 *
 * \param  port    pointer to usif console port.
 * \param  c       character to be written
 *
 * \return None           
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void 
ifx_usif_uart_outb(struct uart_port *port, const char c)
{
    IFX_USIF_TXD_t txd_data;;
    
    /* Wait till the FIFO becomes empty */
    while (IFX_USIF_GET_TX_FILL_FIFO(port) != 0x00){
        ;
    }
    
    txd_data.txd_byte.byte[0] = c;
    IFX_USIF_UART_IRQ_LOCK(port);
    IFX_USIF_PUT_TX_LAST_WORD(IFX_USIF_TXD_BE1, txd_data.txd_word, port); 
    IFX_USIF_UART_IRQ_UNLOCK(port);
    /* Wait till the FIFO becomes empty */
    while ((IFX_USIF_GET_TX_FILL_FIFO(port)) != 0x00){
        ;
    }
}

/**
 * \fn static void ifx_usif_uart_console_write(struct console *co, const char *s, unsigned int count)
 *
 *
 * \brief  This function is used to write the initial debug prints to the console
 *
 * \param  co      pointer to usif console port.
 * \param  s       String to be written
 * \param  count   Number of characters to be written
 *
 * \return None
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static void
ifx_usif_uart_console_write(struct console *co, const char *s, unsigned int count)
{
    u32 i;
    struct uart_port *port;
    unsigned long flags;

    /* is this a valid port */
    if ((co->index == -1) || co->index >= IFX_USIF_UART_NR){
        co->index = 0;
    }

    port  = &ifx_usif_uart_ports[co->index].port;
    
    /*
     * Send each character, until we finish sending all charcaters.
     * At this stage, no interrupts are used, we depend only on the filled FIFO stages.
     * So, if the FIFO is filled, wait till we get little room to write data into the FIFO
     */
    local_irq_save(flags);
    for (i = 0; i < count; i++) {
       /*
        * If a newline character is detected, then we need to send the carriage return
        * character also, followed by the newline character
        */
        if (s[i] == '\n') {
            ifx_usif_uart_outb(port, '\r');
        }        
        ifx_usif_uart_outb(port, s[i]);
    }
    local_irq_restore(flags);
}

/**
 * \fn     static void __init ifx_usif_console_get_options(struct uart_port *port, int *baud,
 *               int *parity, int *bits, int* flow)
 *
 * \brief  This function is used to set initial console setup
 *
 * \param  port    pointer to usif console port.
 * \param  baud    Baudrate
 * \param  parity  Parity setup
 * \param  bits    Number of bits
 * \param  flow    Flowcontrol
 *
 * \return None           
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void __init
ifx_usif_console_get_options(struct uart_port *port, int *baud,
                 int *parity, int *bits, int* flow)
{
    u32 clen;
    u32 prtc_cfg;
    u32 ibaud = IFX_USIF_UART_DEFAULT_BAUDRATE;
    
    prtc_cfg = IFX_USIF_GET_PROTOCOL_CONFIG(port);
 
    *parity = 'n';
    if (prtc_cfg & IFX_USIF_PRTC_CFG_PAR) {
        if (prtc_cfg & IFX_USIF_PRTC_CFG_ODD) {
            *parity = 'o';
        }
        else {
            *parity = 'e';
        }
    }
 
    /* By default, the flow control is disabled */
    *flow = 'n';
 
    /* By default, the character length is 8 */
    clen = MS(prtc_cfg, IFX_USIF_PRTC_CFG_CLEN);
    if ((clen == IFX_USIF_PRTC_CFG_CLEN7) || (clen == IFX_USIF_PRTC_CFG_CLEN8)) {
        *bits = clen;
    }
    else {
        *bits = 0x08;
        printk(KERN_ERR "%s: invalid character length %d", __func__, clen);
    }

    ibaud = ifx_usif_uart_get_baud(port);
    /*
     * This doesn't work exactly because we use integer
     * math to calculate baud which results in rounding
     * errors. Try to make this work for both the fractional 
     * divider and baud counter reload register. Also try to 
     * avoid rounding errors using integer math.
     */
    if (ibaud > 1100 && ibaud < 2300) {
        ibaud = 1200;
    }
    if (ibaud > 2300 && ibaud < 4600) {
        ibaud = 2400;
    }
    if (ibaud > 4600 && ibaud < 9300) {
        ibaud = 4800;
    }
    if (ibaud > 9300 && ibaud < 18600) {
        ibaud = 9600;
    }
    if (ibaud > 18600 && ibaud < 37200) {
        ibaud = 19200;
    }
    if (ibaud > 37200 && ibaud < 55800) {
        ibaud = 38400;
    }
    if (ibaud > 55800 && ibaud < 111700) {
        ibaud = 57600;
    }
    if (ibaud > 111700 && ibaud < 223400) {
        ibaud = 115200;
    }
    *baud = ibaud;
}

/**
 * \fn   static int __init ifx_usif_uart_console_setup(struct console *co, char *options)
 *
 * \brief  This function is used to set default console setup
 *
 * \param  co         pointer to usif console port.
 * \param  options    Setup options
 *
 * \return 0          OK           
 *
 * \ingroup IFX_USIF_UART_FUNCTIONS
 */
static int __init
ifx_usif_uart_console_setup(struct console *co, char *options)
{
    u32 reg;
    struct uart_port *port;
    int baud = IFX_USIF_UART_DEFAULT_BAUDRATE;
    int bits = 8;
    int parity = 'n';
    int flow = 'n';

    /* is this a valid port */
    if ((co->index == -1) || co->index >= IFX_USIF_UART_NR){
        co->index = 0;
    }
    port = &ifx_usif_uart_ports[co->index].port;

    ifx_usif_uart_clc_cfg_mode(port); 

    /* Need not set the USIF baudrate registers, as by default it is set to 115200 */
 
    /*
     * Also, need not set the mode and protocol configuration registers, as by default,
     *  it is set to asynchrounous mode and 8 bit character length
     */
 
    /*
     * Configure TX FIFO, but note that, we will not get any USIF interrupts at startup
     * TX FIFO, non flowcontrol mode, byte aligned, burst size 4 words
     */
    /* Non flow control */  
    reg = IFX_USIF_GET_FIFO_CONFIG(port);
    reg &= ~IFX_USIF_FIFO_CFG_TXFC;
    
    IFX_USIF_SET_FIFO_CONFIG(reg, port);
   
    /* Switch to USIF working mode */
    ifx_usif_uart_clc_run_mode(port);

    /* XXX, Running time configuration */
    if (options) {
        uart_parse_options(options, &baud, &parity, &bits, &flow);
    }
    else {
        ifx_usif_console_get_options(port, &baud, &parity, &bits, &flow);
    }
    
    uart_set_options(port, co, baud, parity, bits, flow);
    return 0;    
}

static struct uart_driver ifx_usif_uart_serial_drv;

static struct console ifx_usif_uart_console = {
    .name        = IFX_USIF_UART_DEV_NAME,
    .write       = ifx_usif_uart_console_write,
    .read        = NULL,
    .device      = uart_console_device,
    .setup       = ifx_usif_uart_console_setup,
    .flags       = CON_PRINTBUFFER,
    .index       = -1,
    .data        = &ifx_usif_uart_serial_drv,
};

/**
 * \fn  static int __init usif_uart_console_init (void)
 *
 * \brief  This is module initialization function for USIF as console
 *
 * \return 0   OK           
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static int __init
usif_uart_console_init (void)
{
    if (ifx_usif_uart_hw_initialized == 0) {
        ifx_usif_uart_hw_init();
    }
    register_console(&ifx_usif_uart_console);
    return 0;
}

console_initcall(usif_uart_console_init);

#endif /* CONFIG_IFX_USIF_UART_CONSOLE */

#ifdef CONFIG_IFX_USIF_UART_CONSOLE
#define USIF_UART_CONSOLE   &ifx_usif_uart_console
#else
#define USIF_UART_CONSOLE   NULL
#endif /* CONFIG_IFX_USIF_UART_CONSOLE */

/** 
 * This structure gives the information about the TTY device driver, which is 
 * used while registering this driver.
 * Initialise it with the device name, major number, minor number etc.
 */
static struct uart_driver ifx_usif_uart_serial_drv = {
    .owner           = THIS_MODULE,
    .driver_name     = IFX_USIF_UART_DEV_NAME,
    .dev_name        = IFX_USIF_UART_DEV_NAME,
    .major           = IFX_USIF_UART_MAJOR,
    .minor           = IFX_USIF_UART_MINOR,  /* Starting minor number */
    .nr              = IFX_USIF_UART_NR,     /* Max number of port */
    .cons            = USIF_UART_CONSOLE,
    .state           = NULL,
    .tty_driver      = NULL,
};

/**
 * \fn static inline int ifx_usif_uart_drv_ver(char *buf)
 * \brief Display USIF/UART driver version after initilazation succeeds
 * 
 * \return number of bytes will be printed
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static inline int
ifx_usif_uart_drv_ver(char *buf)
{
    return ifx_drv_ver(buf, "USIF-UART", IFX_USIF_UART_VER_MAJOR, IFX_USIF_UART_VER_MID, IFX_USIF_UART_VER_MINOR);
}

/**
 * \fn  static inline int  ifx_usif_uart_proc_version(char *buf, char **start, off_t offset, int count, int *eof, void *data)

 *
 * \brief  This function is used read the proc entries used by USIF UART.
 *         Here, the proc entry is used to read version number
 *
 * \param  buf   Proc buffer
 * \param  start  start of the proc entry
 * \param  offset   if set to zero, do not proceed to print proc data
 * \param  count  Maximum proc print size
 * \param  eof    end of the proc entry
 * \param  data   unused
 *
 * \return len    Length of data in buffer
 *
 * \ingroup IFX_USIF_UART_INTERNAL 
 */
static inline int 
ifx_usif_uart_proc_version(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int len = 0;

    /* No sanity check cos length is smaller than one page */
    len += ifx_usif_uart_drv_ver(buf + len);
    *eof = 1;
    return len;    
}

/**
 * \fn static int  ifx_usif_uart_proc_create(void)
 * \brief   Create usif uart proc directory and file when module initialized.
 * 
 * \return  -EFAULT Failed to create proc directory or node
 * \return  0       OK
 * \ingroup IFX_USIF_UART_INTERNAL  
 */
static int 
ifx_usif_uart_proc_create(void)
{
    int ret = 0;
    struct proc_dir_entry *entry;
    
    ifx_usif_uart_proc = proc_mkdir(ifx_usif_uart_proc_node_name, NULL);
    if (ifx_usif_uart_proc == NULL) {
        printk(KERN_ERR "%s:  Unable to create proc entry for %s\n", 
            __func__, ifx_usif_uart_proc_node_name);
        ret = -EFAULT;
        goto done;
    }
    entry = create_proc_read_entry("version",
                            0,
                            ifx_usif_uart_proc,
                            ifx_usif_uart_proc_version,
                            NULL);
    if (entry == NULL) {
        ret = -EFAULT;
        goto err1;
    }

    entry = create_proc_read_entry("uart",
                            0,
                            ifx_usif_uart_proc,
                            ifx_usif_uart_proc_read,
                            NULL);
    if (entry == NULL) {
        ret = -EFAULT;
        goto err2;
    }
    return ret;
err2:
    remove_proc_entry("version", ifx_usif_uart_proc);
err1:
    remove_proc_entry(ifx_usif_uart_proc_node_name, NULL);
done:
    return ret;
}

/**
 * \fn sstatic void ifx_usif_uart_proc_delete(void)
 * \brief Delete usif/uart proc directory and file.
 *
 * \return none
 * \ingroup IFX_USIF_UART_INTERNAL  
 */
static void 
ifx_usif_uart_proc_delete(void)
{
    remove_proc_entry("version", ifx_usif_uart_proc);

    remove_proc_entry("uart", ifx_usif_uart_proc);

    remove_proc_entry(ifx_usif_uart_proc_node_name, NULL);
}

/**
 * \fn     static int __init ifx_usif_uart_init (void)
 *
 * \brief  This is module initialisation function for USIF UART
 *
 * \return -ENOMEM  Failed to register uart driver 
 * \return -EINVAL  Failed to add uart port 
 * \return 0        OK
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static int __init
ifx_usif_uart_init (void)
{
    int ret = 0;
    char ver_str[128] = {0};
    struct uart_port *port;
    
    port = &ifx_usif_uart_ports[0].port;
    
    IFX_USIF_UART_IRQ_LOCK_INIT(port, "USIF-UART");

    /* Console will never release GPIO, initialize again */
    ifx_usif_uart_gpio_init();
    
    if (ifx_usif_uart_hw_initialized == 0) {
        ifx_usif_uart_hw_init();
    }

    /*
     * Register the USIF module as UART device with TTY driver
     * We need to register this, only if the serial driver is not registered already
     */
    ret = uart_register_driver(&ifx_usif_uart_serial_drv);
    if (ret) {
        printk(KERN_ERR "%s: uart_register_driver failed\n", __func__);
        goto err1;
    }
    /* Add the USIF hardware with the TTY driver. */
    ret = uart_add_one_port(&ifx_usif_uart_serial_drv, port);
    if (ret) {
        printk(KERN_ERR "%s: uart_add_one_port failed\n", __func__);
        goto err2;
    }

    ifx_usif_uart_proc_create();

    ifx_usif_uart_drv_ver(ver_str);
    printk(KERN_INFO "%s", ver_str);
    return 0;
err2:
    uart_unregister_driver(&ifx_usif_uart_serial_drv);
err1:
    return ret;
}
module_init(ifx_usif_uart_init);

/**
 * \fn     static void __exit ifx_usif_uart_exit(void)
 *
 * \brief  This is module exit function for USIF UART
 *
 * \return None           
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static void __exit
ifx_usif_uart_exit(void)
{
    struct uart_port *port =  &ifx_usif_uart_ports[0].port;
        
    IFX_USIF_UART_IRQ_LOCK_DESTROY();
    
    /* Exit running mode */
    ifx_usif_uart_clc_cfg_mode(port); 

    uart_remove_one_port(&ifx_usif_uart_serial_drv, port);
    uart_unregister_driver(&ifx_usif_uart_serial_drv);
    
    ifx_usif_uart_proc_delete();
    
    ifx_usif_uart_pm_disable();
    
    usif_uart_gpio_release();
}
module_exit(ifx_usif_uart_exit);

/**
 * \fn     static int ifx_usif_uart_proc_read(char *page, char **start, off_t off, int count,
 *                          int *eof, void *data )
 *
 *
 * \brief  This function is used read the proc entries used by USIF.
 *         Here, the proc entry is used to get the debug information
 *
 *         The major functionality of this is to print the debug information about 
 *         the USIF. This feature could be disabled later which may save little extra
 *         CPU usage. 
 *         To disable this feature, just undefine USIF_UART_PROC_FILE
 *
 * \param  page   Proc buffer
 * \param  start  start of the proc entry
 * \param  off    if set to zero, do not proceed to print proc data
 * \param  count  Maximum proc print size
 * \param  eof    end of the proc entry
 * \param  data   unused
 *
 * \return Length of data in buffer
 *
 * \ingroup IFX_USIF_UART_INTERNAL
 */
static int
ifx_usif_uart_proc_read(char *page, char **start, off_t off, int count,
                            int *eof, void *data )
{
    char *buf = page;
    char *next = buf;
    u32 size = count;
    unsigned long flags;
    u32 t;
    struct uart_port *port = &ifx_usif_uart_ports[0].port;
    IFX_USIF_UART_PORT_t *usif_port_p = (IFX_USIF_UART_PORT_t *)port;
    IFX_USIF_UART_PROC_STAT_t *stat = &(usif_port_p->usif_proc_data);

    if( off != 0 ) {
        return (0);
    }
 
    local_irq_save(flags);
 
    t = scnprintf(next, size, "\nUSIF UART Configuration:\n" );
    if( (t <= 0) || (t > size) ) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "USIF_FDIV_CFG: 0x%x USIF_BC_CFG: 0x%x USIF_ICTMO_CFG: 0x%x\n",
            IFX_USIF_GET_FRAC_DIV(port),
            IFX_USIF_GET_BAUDRATE_COUNTER(port),
            IFX_USIF_GET_INTERCHARACTER_TIMEOUT(port));
    if( (t <= 0) || (t > size) ) {
        goto done;
    }
    size -= t;
    next += t;

    t = scnprintf(next, size, "MOD_CFG: 0x%x PRTC_CFG: 0x%x\n",
            IFX_USIF_GET_MODE_CONFIG(port),
            IFX_USIF_GET_PROTOCOL_CONFIG(port));
    if( (t <= 0) || (t > size) ) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "FIFO_CFG: 0x%x\n",
           IFX_USIF_GET_FIFO_CONFIG(port));
    if( (t <= 0) || (t > size) ) {
        goto done;
    }
    size -= t;
    next += t;
    
    t = scnprintf(next, size, "\nInterrupt Register Status:\n" );
    if( (t <= 0) || (t > size) ) {
        goto done;
    }
    size -= t;
    next += t;

    t = scnprintf(next, size, "RIS: 0x%x  ", IFX_USIF_GET_RAW_INT_STATUS(port));
    if( (t <= 0) || (t > size) ) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "IMIS: 0x%x  ", IFX_USIF_GET_INT_STATUS(port));
    if ((t <= 0) || (t > size)){
        goto done;
    }
    size -= t;
    next += t;

 
    t = scnprintf(next, size, "\nError counts:\n" );
    if((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "RXUR: %d  RXOF: %d TXUR: %d TXOF: %d\n",
            stat->err_int_rxur, port->icount.overrun,
            stat->err_int_txur, stat->err_int_txof);
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "PE: %d FE: %d PHE: %d BRE: %d CRC: %d\n",
            port->icount.parity, port->icount.frame,
            stat->err_int_phe, stat->err_int_bre, stat->err_int_crc);
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "\nModem status signals:\n" );
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "MSS_STAT: 0x%x\n", IFX_USIF_GET_MODEM_STATUS(port));
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
 
    t = scnprintf(next, size, "\nTX statistics:\n" );
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "BREQ: %d LBREQ: %d SREQ: %d LSREQ: %d TXFIN: %d\n",
            stat->tx_intr_breq, stat->tx_intr_lbreq, stat->tx_intr_sreq,
            stat->tx_intr_lsreq, stat->tx_intr_txfin);
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "Start requests: %d  Stop requests: %d Stopped due to flowcontrol: %d\n",
            stat->tx_start_req, stat->tx_stop_req, stat->tx_stopped);
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "Number of writes to the transmit FIFO: %d FIFO xREQ's while no space: %d\n",
            stat->tx_write_fifo, stat->tx_req_fifo_full);
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "Total characters transferred: %d Characters transferred in this session: %d \n", 
            port->icount.tx, (port->icount.tx - stat->tx_bytes));

    if((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "\nRX statistics:\n" );
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "BREQ: %d LBREQ: %d SREQ: %d LSREQ: %d MRPS: %d\n",
            stat->rx_intr_breq, stat->rx_intr_lbreq, stat->rx_intr_sreq,
            stat->rx_intr_lsreq, IFX_USIF_GET_RX_MRPS(port));
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "Low Latency: %d Last RPS read: %d Number of reads to the receive FIFO: %d\n", 
            stat->rx_low_latency, stat->rx_last_rps, stat->rx_read_fifo);
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
 
    t = scnprintf(next, size, "Total characters received: %d Characters received in this session: %d \n", 
            port->icount.rx, (port->icount.rx - stat->rx_bytes));
    if ((t <= 0) || (t > size)) {
        goto done;
    }
    size -= t;
    next += t;
done:
    local_irq_restore(flags);
    *eof = 1;
    return (count - size);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lei Chuanhua, Chuanhua.Lei@infineon.com");
MODULE_DESCRIPTION("Low level serial driver for USIF UART mode");

