/**
** FILE NAME    : ifxmips_hn1_sw_reg.h
** PROJECT      : IFX UEIP
** MODULES      : ETH
** DATE         : 4 Feb. 2011
** AUTHOR       : Yinglei Huang
** DESCRIPTION  : HN1 ETH driver header file
** COPYRIGHT    :       Copyright (c) 2009
**                      Lantiq Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date            $Author         $Comment
** 4 Feb. 2011     Yinglei Huang  Initial release
*******************************************************************************/

#ifndef _IFXMIPS_HN1_REG_H
#define _IFXMIPS_HN1_REG_H 
/*!
  \file ifxmips_hn1_sw_reg.h
  \ingroup IFX_ETH_DRV
  \brief IFX Eth module register definition for VRx platforms
*/

/** ==========================  */
/* Include files                */
/** =========================== */
#include <asm/ifx/ifx_regs.h>

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

#define SW_WRITE_REG32(data,addr)         IFX_REG_W32((data), (volatile unsigned  int *)addr)
#define SW_READ_REG32(addr)               IFX_REG_R32((volatile unsigned  int *)addr)

/** Registers Description */

#define HN1_SWIP_BASE_ADDR                  (0xBE108000)
#define HN1_SWIP_TOP_BASE_ADDR              (HN1_SWIP_BASE_ADDR + (0x0C40 * 4))
/** Switch Reset Control register */
#define ETHSW_SWRES_REG                     (HN1_SWIP_BASE_ADDR)
/** Register Configuration Resets all registers to their default state (such as after a hardware reset).
* 0B RUN reset is off, 1B STOP reset is active */
#define SWRES_R0                            0x0001
/** Hardware Reset Reset all hardware modules except for the register settings.
* 0B RUN reset is off, 1B STOP reset is active */
#define SWRES_R1                            0x0002

/** Ethernet Switch Clock Control Register */
#define ETHSW_CLK_REG                       (HN1_SWIP_BASE_ADDR + (1 * 4))


#define ETHSW_BM_RAM_VAL_3_REG  ( HN1_SWIP_BASE_ADDR + (0x40 * 4) )
#define ETHSW_BM_RAM_VAL_2_REG  ( HN1_SWIP_BASE_ADDR + (0x41 * 4) )
#define ETHSW_BM_RAM_VAL_1_REG  ( HN1_SWIP_BASE_ADDR + (0x42 * 4) )
#define ETHSW_BM_RAM_VAL_0_REG  ( HN1_SWIP_BASE_ADDR + (0x43 * 4) )
#define ETHSW_BM_RAM_ADDR_REG   ( HN1_SWIP_BASE_ADDR + (0x44 * 4) )
#define ETHSW_BM_RAM_CTRL_REG   ( HN1_SWIP_BASE_ADDR + (0x45 * 4) )


/*Buffer manager per port registrs*/
#define ETHSW_BM_PCFG_REG       (HN1_SWIP_BASE_ADDR + (0x80 * 4) )
#define BM_PCFG_CNTEN(val)    (((val & 0x1) << 0 ) )
#define ETHSW_BM_RMON_CTRL_REG  ( HN1_SWIP_BASE_ADDR + (0x81 * 4) )


/** MAC Frame Length Register */
#define MAC_FLEN_REG                        (HN1_SWIP_BASE_ADDR + (0x8C5 * 4) )
#define MAC_FLEN(arg)                       ( (arg & 0x3FFF))

/** MAC Port Status Register */
#define MAC_0_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x900 * 4) )
#define MAC_1_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x90C * 4) )
#define MAC_2_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x918 * 4) )
#define MAC_3_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x924 * 4) )
#define MAC_4_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x930 * 4) )
#define MAC_5_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x93C * 4) )
#define MAC_6_PSTAT_REG                     (HN1_SWIP_BASE_ADDR + (0x948 * 4) )

/** MAC Control Register 0 */
#define MAC_0_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x903 * 4) )
#define MAC_1_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x90F * 4) )
#define MAC_2_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x91B * 4) )
#define MAC_3_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x927 * 4) )
#define MAC_4_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x933 * 4) )
#define MAC_5_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x93F * 4) )
#define MAC_6_CTRL_0                        (HN1_SWIP_BASE_ADDR + (0x94B * 4) )

#define MAC_CTRL_0_FCON_MASK                0x0070
#define MAC_CTRL_0_FCON_AUTO                0x0000
#define MAC_CTRL_0_FCON_RX                  0x0010
#define MAC_CTRL_0_FCON_TX                  0x0020
#define MAC_CTRL_0_FCON_RXTX                0x0030
#define MAC_CTRL_0_FCON_NONE                0x0040

#define MAC_CTRL_0_FDUP_MASK                0x000C
#define MAC_CTRL_0_FDUP_AUTO                0x0000
#define MAC_CTRL_0_FDUP_EN                  0x0004
#define MAC_CTRL_0_FDUP_DIS                 0x000C

#define MAC_CTRL_0_GMII_MASK                0x0003
#define MAC_CTRL_0_GMII_AUTO                0x0000
#define MAC_CTRL_0_GMII_MII                 0x0001
#define MAC_CTRL_0_GMII_RGMII               0x0002

/** Ethernet Switch Fetch DMA Port Control 
 Controls per-port functions of the Fetch DMA */
#define FDMA_PCTRL_PORT6                    (HN1_SWIP_BASE_ADDR + (0xAA4 * 4) )
/** Special Tag Insertion Enable(to egress frames )*/
#define FDMA_PCTRL_STEN                     (1 << 1)

/** HN1 Switch Subsystem Top Level Registers */

/** Global Control Register 0 */
#define GLOB_CTRL_REG                       (HN1_SWIP_TOP_BASE_ADDR)
/** Global Software Reset Reset all hardware modules excluding the register settings.
* 0B OFF reset is off, 1B ON reset is active */
#define GLOB_CTRL_SWRES                     0x0001
/** Global Hardware Reset Reset all hardware modules including the register settings.
* 0B OFF reset is off, 1B ON reset is active */
#define GLOB_CTRL_HWRES                     0x0002
/** Global Switch Macro Enable If set to OFF, the switch macro is inactive and frame forwarding is disabled.
* 0B OFF switch macro is not active, 1B ON switch macro is active */
#define GLOB_CTRL_SE                        0x8000

/** MDIO Control Register */
#define MDIO_CTRL_REG                       (HN1_SWIP_TOP_BASE_ADDR + (8 * 4))
/** MDIO Busy*/
#define MDIO_CTRL_MBUSY                     0x1000
#define MDIO_CTRL_OP_MASK                   0x0C00
#define MDIO_CTRL_OP_WR                     0x0400
#define MDIO_CTRL_OP_RD                     0x0800
#define MDIO_CTRL_PHYAD_SET(arg)            ((arg & 0x1F) << 5)
#define MDIO_CTRL_PHYAD_GET(arg)            ( (arg >> 5 ) & 0x1F)
#define MDIO_CTRL_REGAD(arg)                ( arg & 0x1F)

/** MDIO Read Data Register */
#define MDIO_READ_REG                       (HN1_SWIP_TOP_BASE_ADDR + (9 * 4))
#define MDIO_READ_RDATA(arg)                (arg & 0xFFFF)

/** MDIO Write Data Register */
#define MDIO_WRITE_REG                      (HN1_SWIP_TOP_BASE_ADDR + (0x0A * 4))
#define MDIO_READ_WDATA(arg)                (arg & 0xFFFF)

/** MDC Clock Configuration Register 0 */
#define MDC_CFG_0_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x0B * 4))
#define MDC_CFG_0_PEN_SET(port)             (0x1 << port )
#define MDC_CFG_0_PEN_GET(port, reg_data)   ((reg_data >> port ) & 0x1 )
/** MDC Clock Configuration Register 1 */
#define MDC_CFG_1_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x0C * 4))

/** PHY Address Register PORT 5~0 */
#define PHY_ADDR_5                          (HN1_SWIP_TOP_BASE_ADDR + (0x10 * 4))
#define PHY_ADDR_4                          (HN1_SWIP_TOP_BASE_ADDR + (0x11 * 4))
#define PHY_ADDR_3                          (HN1_SWIP_TOP_BASE_ADDR + (0x12 * 4))
#define PHY_ADDR_2                          (HN1_SWIP_TOP_BASE_ADDR + (0x13 * 4))
#define PHY_ADDR_1                          (HN1_SWIP_TOP_BASE_ADDR + (0x14 * 4))
#define PHY_ADDR_0                          (HN1_SWIP_TOP_BASE_ADDR + (0x15 * 4))
/** Link Status Control */
#define PHY_ADDR_LINKST_MASK                0x6000
#define PHY_ADDR_LINKST_AUTO                0x0000
#define PHY_ADDR_LINKST_UP                  0x2000
#define PHY_ADDR_LINKST_DOWN                0x4000
/** Speed Control */
#define PHY_ADDR_SPEED_MASK                 0x1800
#define PHY_ADDR_SPEED_10                   0x0000
#define PHY_ADDR_SPEED_100                  0x0800
#define PHY_ADDR_SPEED_1000                 0x1000
#define PHY_ADDR_SPEED_AUTO                 0x1800
/** Full Duplex Control */
#define PHY_ADDR_FDUP_MASK                  0x0600
#define PHY_ADDR_FDUP_AUTO                  0x0000
#define PHY_ADDR_FDUP_EN                    0x0200
#define PHY_ADDR_FDUP_DIS                   0x0600
/** Flow Control Mode TX */
#define PHY_ADDR_FCONTX_MASK                0x0180
#define PHY_ADDR_FCONTX_AUTO                0x0000
#define PHY_ADDR_FCONTX_EN                  0x0080
#define PHY_ADDR_FCONTX_DIS                 0x0180
/** Flow Control Mode RX */
#define PHY_ADDR_FCONRX_MASK                0x0060
#define PHY_ADDR_FCONRX_AUTO                0x0000
#define PHY_ADDR_FCONRX_EN                  0x0020
#define PHY_ADDR_FCONRX_DIS                 0x0060
/** PHY Address */
#define PHY_ADDR_ADDR(arg)                  (arg & 0x1F)

/** PHY MDIO Polling Status per PORT */
#define MDIO_STAT_0_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x16 * 4))
#define MDIO_STAT_1_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x17 * 4))
#define MDIO_STAT_2_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x18 * 4))
#define MDIO_STAT_3_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x19 * 4))
#define MDIO_STAT_4_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x1A * 4))
#define MDIO_STAT_5_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x1B * 4))
/** PHY Active Status */
#define MDIO_STAT_PACT                      0x0040
#define MDIO_STAT_LSTAT                     0x0020
#define MDIO_STAT_SPEED(arg)                ( (arg >> 0x3) & 0x03)
#define MDIO_STAT_FDUP                      0x0004
#define MDIO_STAT_RXPAUEN                   0x0002
#define MDIO_STAT_TXPAUEN                   0x0001

/** xMII Control Registers */
/** xMII Port 0 Configuration register */
#define MII_CFG_0_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x36 * 4))
#define MII_CFG_1_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x38 * 4))
#define MII_CFG_2_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x3A * 4))
#define MII_CFG_3_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x3C * 4))
#define MII_CFG_4_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x3E * 4))
#define MII_CFG_5_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x40 * 4))
#define MII_CFG_RES                         0x8000
#define MII_CFG_EN                          0x4000
/** Bits are only valid in PHY Mode */
#define MII_CFG_CRS_SET(arg)                ( (arg & 0x03) << 0x9)
#define MII_CFG_CRS_GET(arg)                ( (arg >> 0x9) & 0x03)
/** RGMII In Band Status */
#define MII_CFG_RGMII_IBS                   0x0100
/** RMII Reference Clock Direction of the Port */
#define MII_CFG_RMII_OUT                    0x0080
/** xMII Port Interface Clock Rate */
#define MII_CFG_MIIRATE_MASK                0x0070
#define MII_CFG_MIIRATE_2_5MHZ              0x0000
#define MII_CFG_MIIRATE_25MHZ               0x0010
#define MII_CFG_MIIRATE_125MHZ              0x0020
#define MII_CFG_MIIRATE_50MHZ               0x0030
#define MII_CFG_MIIRATE_AUTO                0x0040
/** xMII Interface Mode */
#define MII_CFG_MIIMODE_MASK                0x000F
#define MII_CFG_MIIMODE_MIIP                0x0000
#define MII_CFG_MIIMODE_MIIM                0x0001
#define MII_CFG_MIIMODE_RMIIP               0x0002
#define MII_CFG_MIIMODE_RMIIM               0x0003
#define MII_CFG_MIIMODE_RGMII               0x0004

/** Configuration of Clock Delay for Port 0 (used for RGMII mode only)*/
#define MII_PCDU_0_REG                      (HN1_SWIP_TOP_BASE_ADDR + (0x37 * 4))
#define MII_PCDU_1_REG                      (HN1_SWIP_TOP_BASE_ADDR + (0x39 * 4))
#define MII_PCDU_5_REG                      (HN1_SWIP_TOP_BASE_ADDR + (0x41 * 4))
#define MII_PCDU_RXLOCK                     0x8000
#define MII_PCDU_TXLOCK                     0x4000
#define MII_PCDU_RXSEL_CLK_MASK             0x3000
#define MII_PCDU_RXSEL_CLK_AUTO             0x0000
#define MII_PCDU_RXSEL_CLK_RXCLK            0x1000
#define MII_PCDU_RXSEL_CLK_CLKREF           0x2000
#define MII_PCDU_RXINIT                     0x0800
#define MII_PCDU_RXPD                       0x0400
#define MII_PCDU_RXDLY_MASK                 0x0380

#define MII_PCDU_TXSEL_CLK_MASK             0x0060
#define MII_PCDU_TXSEL_CLK_AUTO             0x0000
#define MII_PCDU_TXSEL_CLK_TXCLK            0x0020
#define MII_PCDU_TXSEL_CLK_CLKREF           0x0040
#define MII_PCDU_TXINIT                     0x0010
#define MII_PCDU_TXPD                       0x0008
#define MII_PCDU_TXDLY_MASK                 0x0007

/** PMAC Header Control Register */
#define PMAC_HD_CTL_REG                    (HN1_SWIP_TOP_BASE_ADDR + (0x82 * 4))
#define PMAC_HD_CTL_FC                      0x0400
#define PMAC_HD_CTL_CCRC                    0x0200
#define PMAC_HD_CTL_RST                     0x0100
#define PMAC_HD_CTL_AST                     0x0080
#define PMAC_HD_CTL_RXSH                    0x0040
#define PMAC_HD_CTL_RL2                     0x0020
#define PMAC_HD_CTL_RC                      0x0010
#define PMAC_HD_CTL_AS                      0x0008
#define PMAC_HD_CTL_AC                      0x0004
#define PMAC_HD_CTL_TAG                     0x0002
#define PMAC_HD_CTL_ADD                     0x0001

/** PMAC Type/Length register */
#define PMAC_TL_REG                         (HN1_SWIP_TOP_BASE_ADDR + (0x83 * 4))
/** PMAC Source Address Register */
#define PMAC_SA1_REG                        (HN1_SWIP_TOP_BASE_ADDR + (0x84 * 4))
#define PMAC_SA2_REG                        (HN1_SWIP_TOP_BASE_ADDR + (0x85 * 4))
#define PMAC_SA3_REG                        (HN1_SWIP_TOP_BASE_ADDR + (0x86 * 4))
/** PMAC Destination Address Register */
#define PMAC_DA1_REG                        (HN1_SWIP_TOP_BASE_ADDR + (0x87 * 4))
#define PMAC_DA2_REG                        (HN1_SWIP_TOP_BASE_ADDR + (0x88 * 4))
#define PMAC_DA3_REG                        (HN1_SWIP_TOP_BASE_ADDR + (0x89 * 4))
/** PMAC VLAN register */
#define PMAC_VLAN_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x8A * 4))
/** PMAC Inter Packet Gap in RX Direction */
#define PMAC_RX_IPG_REG                     (HN1_SWIP_TOP_BASE_ADDR + (0x8B * 4))
/** PMAC Special Tag Ethertype */
#define PMAC_ST_ETYPE_REG                   (HN1_SWIP_TOP_BASE_ADDR + (0x8C * 4))
/** PMAC Ethernet WAN Group */
#define PMAC_EWAN_REG                       (HN1_SWIP_TOP_BASE_ADDR + (0x8D * 4))

#endif /*_IFXMIPS_HN1_REG_H  */
