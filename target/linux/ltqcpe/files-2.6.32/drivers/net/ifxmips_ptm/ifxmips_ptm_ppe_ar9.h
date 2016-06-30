/******************************************************************************
**
** FILE NAME    : ifxmips_ptm_ppe_ar9.h
** PROJECT      : UEIP
** MODULES      : PTM
**
** DATE         : 7 Jul 2009
** AUTHOR       : Xu Liang
** DESCRIPTION  : PTM driver header file (PPE register for AR9)
** COPYRIGHT    :       Copyright (c) 2006
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
** 07 JUL 2009  Xu Liang        Init Version
*******************************************************************************/



#ifndef IFXMIPS_PTM_PPE_AR9_H
#define IFXMIPS_PTM_PPE_AR9_H



/*
 *  FPI Configuration Bus Register and Memory Address Mapping
 */
#define IFX_PPE                         (KSEG1 | 0x1E180000)
#define PP32_DEBUG_REG_ADDR(i, x)       ((volatile unsigned int*)(IFX_PPE + (((x) + 0x0000) << 2)))
#define PPM_INT_REG_ADDR(i, x)          ((volatile unsigned int*)(IFX_PPE + (((x) + 0x0030) << 2)))
#define PP32_INTERNAL_RES_ADDR(i, x)    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x0040) << 2)))
#define CDM_CODE_MEMORY(i, x)           ((volatile unsigned int*)(IFX_PPE + (((x) + 0x1000) << 2)))
#define PPE_REG_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0x4000) << 2)))
#define CDM_DATA_MEMORY(i, x)           ((volatile unsigned int*)(IFX_PPE + (((x) + 0x5000) << 2)))
#define PPM_INT_UNIT_ADDR(x)            ((volatile unsigned int*)(IFX_PPE + (((x) + 0x6000) << 2)))
#define PPM_TIMER0_ADDR(x)              ((volatile unsigned int*)(IFX_PPE + (((x) + 0x6100) << 2)))
#define PPM_TASK_IND_REG_ADDR(x)        ((volatile unsigned int*)(IFX_PPE + (((x) + 0x6200) << 2)))
#define PPS_BRK_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0x6300) << 2)))
#define PPM_TIMER1_ADDR(x)              ((volatile unsigned int*)(IFX_PPE + (((x) + 0x6400) << 2)))
#define SB_RAM0_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0x8000) << 2)))
#define SB_RAM1_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0x8800) << 2)))
#define SB_RAM2_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0x9000) << 2)))
#define SB_RAM3_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0x9800) << 2)))
#define SB_RAM4_ADDR(x)                 ((volatile unsigned int*)(IFX_PPE + (((x) + 0xA000) << 2)))
#define QSB_CONF_REG_ADDR(x)            ((volatile unsigned int*)(IFX_PPE + (((x) + 0xC000) << 2)))

/*
 *  DWORD-Length of Memory Blocks
 */
#define PP32_DEBUG_REG_DWLEN            0x0030
#define PPM_INT_REG_DWLEN               0x0010
#define PP32_INTERNAL_RES_DWLEN         0x00C0
#define CDM_CODE_MEMORYn_DWLEN(n)       0x1000
#define PPE_REG_DWLEN                   0x1000
#define CDM_DATA_MEMORY_DWLEN           CDM_CODE_MEMORYn_DWLEN(1)
#define PPM_INT_UNIT_DWLEN              0x0100
#define PPM_TIMER0_DWLEN                0x0100
#define PPM_TASK_IND_REG_DWLEN          0x0100
#define PPS_BRK_DWLEN                   0x0100
#define PPM_TIMER1_DWLEN                0x0100
#define SB_RAM0_DWLEN                   0x0800
#define SB_RAM1_DWLEN                   0x0800
#define SB_RAM2_DWLEN                   0x0800
#define SB_RAM3_DWLEN                   0x0800
#define SB_RAM4_DWLEN                   0x0C00
#define QSB_CONF_REG_DWLEN              0x0100

/*
 *  PP32 to FPI Address Mapping
 */
#define SB_BUFFER(__sb_addr)            ((volatile unsigned int *)((((__sb_addr) >= 0x0000) && ((__sb_addr) <= 0x0FFF)) ? PP32_DEBUG_REG_ADDR(0, (__sb_addr)):   \
                                                                   (((__sb_addr) >= 0x2000) && ((__sb_addr) <= 0x27FF)) ? SB_RAM0_ADDR((__sb_addr) - 0x2000) :   \
                                                                   (((__sb_addr) >= 0x2800) && ((__sb_addr) <= 0x2FFF)) ? SB_RAM1_ADDR((__sb_addr) - 0x2800) :   \
                                                                   (((__sb_addr) >= 0x3000) && ((__sb_addr) <= 0x37FF)) ? SB_RAM2_ADDR((__sb_addr) - 0x3000) :   \
                                                                   (((__sb_addr) >= 0x3800) && ((__sb_addr) <= 0x3FFF)) ? SB_RAM3_ADDR((__sb_addr) - 0x3800) :   \
                                                                   (((__sb_addr) >= 0x4000) && ((__sb_addr) <= 0x4BFF)) ? SB_RAM4_ADDR((__sb_addr) - 0x4000) :   \
                                                                0))

/*
 *  PP32 Debug Control Register
 */
#define NUM_OF_PP32                             1

#define PP32_DBG_CTRL(n)                        PP32_DEBUG_REG_ADDR(n, 0x0000)

#define DBG_CTRL_RESTART                        0
#define DBG_CTRL_STOP                           1

#define PP32_CTRL_CMD(n)                        PP32_DEBUG_REG_ADDR(n, 0x0B00)
  #define PP32_CTRL_CMD_RESTART                 (1 << 0)
  #define PP32_CTRL_CMD_STOP                    (1 << 1)
  #define PP32_CTRL_CMD_STEP                    (1 << 2)
  #define PP32_CTRL_CMD_BREAKOUT                (1 << 3)

#define PP32_CTRL_OPT(n)                        PP32_DEBUG_REG_ADDR(n, 0x0C00)
  #define PP32_CTRL_OPT_BREAKOUT_ON_STOP_ON     (3 << 0)
  #define PP32_CTRL_OPT_BREAKOUT_ON_STOP_OFF    (2 << 0)
  #define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN_ON  (3 << 2)
  #define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN_OFF (2 << 2)
  #define PP32_CTRL_OPT_STOP_ON_BREAKIN_ON      (3 << 4)
  #define PP32_CTRL_OPT_STOP_ON_BREAKIN_OFF     (2 << 4)
  #define PP32_CTRL_OPT_STOP_ON_BREAKPOINT_ON   (3 << 6)
  #define PP32_CTRL_OPT_STOP_ON_BREAKPOINT_OFF  (2 << 6)
  #define PP32_CTRL_OPT_BREAKOUT_ON_STOP(n)     (*PP32_CTRL_OPT(n) & (1 << 0))
  #define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN(n)  (*PP32_CTRL_OPT(n) & (1 << 2))
  #define PP32_CTRL_OPT_STOP_ON_BREAKIN(n)      (*PP32_CTRL_OPT(n) & (1 << 4))
  #define PP32_CTRL_OPT_STOP_ON_BREAKPOINT(n)   (*PP32_CTRL_OPT(n) & (1 << 6))

#define PP32_BRK_PC(n, i)                       PP32_DEBUG_REG_ADDR(n, 0x0900 + (i) * 2)
#define PP32_BRK_PC_MASK(n, i)                  PP32_DEBUG_REG_ADDR(n, 0x0901 + (i) * 2)
#define PP32_BRK_DATA_ADDR(n, i)                PP32_DEBUG_REG_ADDR(n, 0x0904 + (i) * 2)
#define PP32_BRK_DATA_ADDR_MASK(n, i)           PP32_DEBUG_REG_ADDR(n, 0x0905 + (i) * 2)
#define PP32_BRK_DATA_VALUE_RD(n, i)            PP32_DEBUG_REG_ADDR(n, 0x0908 + (i) * 2)
#define PP32_BRK_DATA_VALUE_RD_MASK(n, i)       PP32_DEBUG_REG_ADDR(n, 0x0909 + (i) * 2)
#define PP32_BRK_DATA_VALUE_WR(n, i)            PP32_DEBUG_REG_ADDR(n, 0x090C + (i) * 2)
#define PP32_BRK_DATA_VALUE_WR_MASK(n, i)       PP32_DEBUG_REG_ADDR(n, 0x090D + (i) * 2)
  #define PP32_BRK_CONTEXT_MASK(i)              (1 << (i))
  #define PP32_BRK_CONTEXT_MASK_EN              (1 << 4)
  #define PP32_BRK_COMPARE_GREATER_EQUAL        (1 << 5)    //  valid for break data value rd/wr only
  #define PP32_BRK_COMPARE_LOWER_EQUAL          (1 << 6)
  #define PP32_BRK_COMPARE_EN                   (1 << 7)

#define PP32_BRK_TRIG(n)                        PP32_DEBUG_REG_ADDR(n, 0x0F00)
  #define PP32_BRK_GRPi_PCn_ON(i, n)            ((3 << ((n) * 2)) << ((i) * 16))
  #define PP32_BRK_GRPi_PCn_OFF(i, n)           ((2 << ((n) * 2)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_ADDRn_ON(i, n)     ((3 << ((n) * 2 + 4)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_ADDRn_OFF(i, n)    ((2 << ((n) * 2 + 4)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_RDn_ON(i, n) ((3 << ((n) * 2 + 8)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_RDn_OFF(i, n)((2 << ((n) * 2 + 8)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_WRn_ON(i, n) ((3 << ((n) * 2 + 12)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_WRn_OFF(i, n)((2 << ((n) * 2 + 12)) << ((i) * 16))
  #define PP32_BRK_GRPi_PCn(k, i, n)            (*PP32_BRK_TRIG(k) & ((1 << ((n))) << ((i) * 8)))
  #define PP32_BRK_GRPi_DATA_ADDRn(k, i, n)     (*PP32_BRK_TRIG(k) & ((1 << ((n) + 2)) << ((i) * 8)))
  #define PP32_BRK_GRPi_DATA_VALUE_RDn(k, i, n) (*PP32_BRK_TRIG(k) & ((1 << ((n) + 4)) << ((i) * 8)))
  #define PP32_BRK_GRPi_DATA_VALUE_WRn(k, i, n) (*PP32_BRK_TRIG(k) & ((1 << ((n) + 6)) << ((i) * 8)))

#define PP32_CPU_STATUS(n)                      PP32_DEBUG_REG_ADDR(n, 0x0D00)
#define PP32_HALT_STAT(n)                       PP32_CPU_STATUS(n)
#define PP32_DBG_CUR_PC(n)                      PP32_CPU_STATUS(n)
  #define PP32_CPU_USER_STOPPED(n)              (*PP32_CPU_STATUS(n) & (1 << 0))
  #define PP32_CPU_USER_BREAKIN_RCV(n)          (*PP32_CPU_STATUS(n) & (1 << 1))
  #define PP32_CPU_USER_BREAKPOINT_MET(n)       (*PP32_CPU_STATUS(n) & (1 << 2))
  #define PP32_CPU_CUR_PC(n)                    (*PP32_CPU_STATUS(n) >> 16)

#define PP32_BREAKPOINT_REASONS(n)              PP32_DEBUG_REG_ADDR(n, 0x0A00)
  #define PP32_BRK_PC_MET(n, i)                 (*PP32_BREAKPOINT_REASONS(n) & (1 << (i)))
  #define PP32_BRK_DATA_ADDR_MET(n, i)          (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) + 2)))
  #define PP32_BRK_DATA_VALUE_RD_MET(n, i)      (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) + 4)))
  #define PP32_BRK_DATA_VALUE_WR_MET(n, i)      (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) + 6)))
  #define PP32_BRK_DATA_VALUE_RD_LO_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 8)))
  #define PP32_BRK_DATA_VALUE_RD_GT_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 9)))
  #define PP32_BRK_DATA_VALUE_WR_LO_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 12)))
  #define PP32_BRK_DATA_VALUE_WR_GT_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 13)))
  #define PP32_BRK_CUR_CONTEXT(n)               ((*PP32_BREAKPOINT_REASONS(n) >> 16) & 0x03)

#define PP32_GP_REG_BASE(n)                     PP32_DEBUG_REG_ADDR(n, 0x0E00)
#define PP32_GP_CONTEXTi_REGn(n, i, j)          PP32_DEBUG_REG_ADDR(n, 0x0E00 + (i) * 16 + (j))

/*
 *  Share Buffer Registers
 */
#define SB_MST_PRI0                     PPE_REG_ADDR(0x0300)
#define SB_MST_PRI1                     PPE_REG_ADDR(0x0301)

/*
 *  EMA Registers
 */
#define EMA_CMDCFG                      PPE_REG_ADDR(0x0A00)
#define EMA_DATACFG                     PPE_REG_ADDR(0x0A01)
#define EMA_CMDCNT                      PPE_REG_ADDR(0x0A02)
#define EMA_DATACNT                     PPE_REG_ADDR(0x0A03)
#define EMA_ISR                         PPE_REG_ADDR(0x0A04)
#define EMA_IER                         PPE_REG_ADDR(0x0A05)
#define EMA_CFG                         PPE_REG_ADDR(0x0A06)
#define EMA_SUBID                       PPE_REG_ADDR(0x0A07)

#define EMA_ALIGNMENT                   4

/*
 *  DPlus Registers
 */
#define DM_RXDB                         PPE_REG_ADDR(0x0612)
#define DM_RXCB                         PPE_REG_ADDR(0x0613)
#define DM_RXCFG                        PPE_REG_ADDR(0x0614)
#define DM_RXPGCNT                      PPE_REG_ADDR(0x0615)
#define DM_RXPKTCNT                     PPE_REG_ADDR(0x0616)
#define DS_RXDB                         PPE_REG_ADDR(0x0710)
#define DS_RXCB                         PPE_REG_ADDR(0x0711)
#define DS_RXCFG                        PPE_REG_ADDR(0x0712)
#define DS_RXPGCNT                      PPE_REG_ADDR(0x0713)

/*
 *  3-Port Switch Registers (partial)
 */
#define IFX_SW                          (KSEG1 | 0x1E108000)
#define SW_REG(off)                     ((volatile unsigned int*)(IFX_SW + (off)))
#define SW_P2_CTL                       SW_REG(0x00C)


/*
 *  Mailbox IGU1 Interrupt
 */
#ifdef CONFIG_AR10
  #define PPE_MAILBOX_IGU1_INT          get_mailbox1_irq_num()
#else
  #define PPE_MAILBOX_IGU1_INT          INT_NUM_IM2_IRL24
#endif

#ifdef CONFIG_AR10
/*
 *CHIP ID 
 */
struct ar10_chipid{
    unsigned int   res0     :1;
    unsigned int   version  :3;
    unsigned int   pnum     :16;
    unsigned int   manid    :11;
    unsigned int   res1     :1;
};

/*
  * Chip ID Registers 
*/
#define AR10_MIPS_CHIPID_BASE                   (KSEG1 | 0x1F107340)
#define AR10_MIPS_CHIPID                        (volatile u32*)(AR10_MIPS_CHIPID_BASE + 0x4)

extern uint32_t get_mailbox1_irq_num(void);

#endif



#endif  //  IFXMIPS_PTM_PPE_AR9_H
