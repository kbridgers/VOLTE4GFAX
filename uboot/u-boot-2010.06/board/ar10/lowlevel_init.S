
/*
 *  Memory sub-system initialization code for INCA-IP2 development board.
 *  Andre Messerschmidt
 *  Copyright (c) 2005  Infineon Technologies AG 
 *
 *  Based on Inca-IP code 
 *  Copyright (c) 2003  Wolfgang Denk <wd@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/* History:
      Wu Qi Ming Sep 18, 2009 initial version
 */
#include <config.h>
#include <version.h>
#include <asm/regdef.h>


#define DDR_OFFSET   0xbf801000
#define DDR_CCR00      (DDR_OFFSET + 0x0000 )
#define DDR_CCR01      (DDR_OFFSET + 0x0010 )
#define DDR_CCR02      (DDR_OFFSET + 0x0020 )
#define DDR_CCR03      (DDR_OFFSET + 0x0030 )
#define DDR_CCR04      (DDR_OFFSET + 0x0040 )
#define DDR_CCR05      (DDR_OFFSET + 0x0050 )
#define DDR_CCR06      (DDR_OFFSET + 0x0060 )
#define DDR_CCR07      (DDR_OFFSET + 0x0070 )
#define DDR_CCR08      (DDR_OFFSET + 0x0080 )
#define DDR_CCR09      (DDR_OFFSET + 0x0090 )
#define DDR_CCR10      (DDR_OFFSET + 0x00A0 )
#define DDR_CCR11      (DDR_OFFSET + 0x00B0 )
#define DDR_CCR12      (DDR_OFFSET + 0x00C0 )
#define DDR_CCR13      (DDR_OFFSET + 0x00D0 )
#define DDR_CCR14      (DDR_OFFSET + 0x00E0 )
#define DDR_CCR15      (DDR_OFFSET + 0x00F0 )
#define DDR_CCR16      (DDR_OFFSET + 0x0100 )
#define DDR_CCR17      (DDR_OFFSET + 0x0110 )
#define DDR_CCR18      (DDR_OFFSET + 0x0120 )
#define DDR_CCR19      (DDR_OFFSET + 0x0130 )
#define DDR_CCR20      (DDR_OFFSET + 0x0140 )
#define DDR_CCR21      (DDR_OFFSET + 0x0150 )
#define DDR_CCR22      (DDR_OFFSET + 0x0160 )
#define DDR_CCR23      (DDR_OFFSET + 0x0170 )
#define DDR_CCR24      (DDR_OFFSET + 0x0180 )
#define DDR_CCR25      (DDR_OFFSET + 0x0190 )
#define DDR_CCR26      (DDR_OFFSET + 0x01A0 )
#define DDR_CCR27      (DDR_OFFSET + 0x01B0 )
#define DDR_CCR28      (DDR_OFFSET + 0x01C0 )
#define DDR_CCR29      (DDR_OFFSET + 0x01D0 )
#define DDR_CCR30      (DDR_OFFSET + 0x01E0 )
#define DDR_CCR31      (DDR_OFFSET + 0x01F0 )
#define DDR_CCR32      (DDR_OFFSET + 0x0200 )
#define DDR_CCR33      (DDR_OFFSET + 0x0210 )
#define DDR_CCR34      (DDR_OFFSET + 0x0220 )
#define DDR_CCR35      (DDR_OFFSET + 0x0230 )
#define DDR_CCR36      (DDR_OFFSET + 0x0240 )
#define DDR_CCR37      (DDR_OFFSET + 0x0250 )
#define DDR_CCR38      (DDR_OFFSET + 0x0260 )
#define DDR_CCR48      (DDR_OFFSET + 0x0270 )
#define DDR_CCR49      (DDR_OFFSET + 0x0280 )
#define DDR_CCR50      (DDR_OFFSET + 0x0290 )
#define DDR_CCR51      (DDR_OFFSET + 0x02A0 )
#define DDR_CCR54      (DDR_OFFSET + 0x02B0 )
#define DDR_CCR55      (DDR_OFFSET + 0x02C0 )
#define DDR_PHYR0      (DDR_OFFSET + 0x0400 )
#define DDR_PHYR1      (DDR_OFFSET + 0x0410 )
#define DDR_PHYR2      (DDR_OFFSET + 0x0420 )
#define DDR_PHYR3      (DDR_OFFSET + 0x0430 )
#define DDR_PHYR4      (DDR_OFFSET + 0x0440 )
#define DDR_PHYR5      (DDR_OFFSET + 0x0450 )
#define DDR_PHYR6      (DDR_OFFSET + 0x0460 )
#define DDR_PHYR7      (DDR_OFFSET + 0x0470 )
#define DDR_PHYR8      (DDR_OFFSET + 0x0480 )
#define DDR_PHYR9      (DDR_OFFSET + 0x0490 )
#define DDR_PHYR10     (DDR_OFFSET + 0x04A0 )
#define DDR_PHYR11     (DDR_OFFSET + 0x04B0 )
#define DDR_PHYR12     (DDR_OFFSET + 0x04C0 )
#define DDR_PHYR13     (DDR_OFFSET + 0x04D0 )
#define DDR_PHYR14     (DDR_OFFSET + 0x04E0 ) 
#define DDR_PHYR15     (DDR_OFFSET + 0x04F0 )


#ifdef CONFIG_AR10_CPU_125M_RAM_125M
   #define CONFIG_AR10_CGU_SYS_VALUE    0x22
#elif defined(CONFIG_AR10_CPU_250M_RAM_125M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x12
#elif defined(CONFIG_AR10_CPU_250M_RAM_250M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x11
#elif defined(CONFIG_AR10_CPU_500M_RAM_250M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x01
#elif defined(CONFIG_AR10_CPU_600M_RAM_150M) 
   #define CONFIG_AR10_CGU_SYS_VALUE    0x102
#elif defined(CONFIG_AR10_CPU_600M_RAM_300M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x101
#elif defined(CONFIG_AR10_CPU_150M_RAM_150M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x122
#elif defined(CONFIG_AR10_CPU_300M_RAM_150M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x112
#elif defined(CONFIG_AR10_CPU_300M_RAM_300M)
   #define CONFIG_AR10_CGU_SYS_VALUE    0x111
#elif defined(CONFIG_GRX390_CPU_666M_RAM_333M)
   #define CONFIG_AR10_CGU_SYS_VALUE	0x201
#elif defined(CONFIG_GRX390_CPU_180M_RAM_180M)
   #define CONFIG_AR10_CGU_SYS_VALUE	0x422
#elif defined(CONFIG_GRX390_CPU_360M_RAM_180M)
   #define CONFIG_AR10_CGU_SYS_VALUE	0x412
#elif defined(CONFIG_GRX390_CPU_360M_RAM_360M)
   #define CONFIG_AR10_CGU_SYS_VALUE	0x411
#elif defined(CONFIG_GRX390_CPU_720M_RAM_360M)
   #define CONFIG_AR10_CGU_SYS_VALUE	0x401
#else
      /*fix me, put correct values here*/
   #define CONFIG_AR10_CGU_SYS_VALUE    0x13
#endif

#ifdef CONFIG_AR10_PPE_FREQ_250M
   #define CONFIG_AR10_CGU_CLKFSR_VALUE 0x10010000
#else /*400M*/
   #define CONFIG_AR10_CGU_CLKFSR_VALUE 0x10040000
#endif

#ifdef CONFIG_AR10_CRYSTAL_25M /*25M crystal clock*/
    #define CONFIG_AR10_PLL0_CFG_VALUE 0x00F018D1   /*default value for 25M crystal*/
#else
    //#define CONFIG_AR10_PLL0_CFG_VALUE 0x00B01F21
	/*M=2, N(6:0)=49, N(7)=1, */
	/*setting bit 7 of N means the final pll clk frequency should be multiplied by 1.66*/
	/*bit 7 itself should not be considered as part of N value*/
	#define CONFIG_AR10_PLL0_CFG_VALUE 0x00B02C49
    /*M=5, N(6:0)=99, N(7)=1*/
	//#define CONFIG_AR10_PLL0_CFG_VALUE 0x00B038D5
#endif


#define CGU_OFFSET  0xBF103000
#define PLL0_CFG    (CGU_OFFSET + 0x0004)
#define PLL1_CFG    (CGU_OFFSET + 0x0008)
#define CGU_SYS     (CGU_OFFSET + 0x000C)
#define CGU_CLKFSR  (CGU_OFFSET + 0x0010)
#define CGU_UPDATE  (CGU_OFFSET + 0x0020)
#define IF_CLK      (CGU_OFFSET + 0x0024)

#define RCU_OFFSET   0xBF203000
#define RST_REQ      (RCU_OFFSET + 0x0010)


  .set  noreorder

/*
 * void cgu_init(long)
 */
  .globl  cgu_init
  .ent  cgu_init
cgu_init:
    li  t2, CGU_SYS
    lw  t2, 0(t2)
    bne t2, CONFIG_AR10_CGU_SYS_VALUE, 1f /*check if CGU_SYS is correct*/
    nop

    li  t2, CGU_CLKFSR
    lw  t2, 0(t2)
    bne t2, CONFIG_AR10_CGU_CLKFSR_VALUE, 1f /*check if CGU_CLKFSR is correct*/
    nop
					
freq_up2date:
    j ra
	nop
		
1:
   li  t2, CGU_SYS
   li  a0, CONFIG_AR10_CGU_SYS_VALUE
   addi a0, a0, 0x80
   sw  a0, 0(t2)    /*store predefined value to CGU_SYS*/

   li  t2, CGU_CLKFSR
   li  a0, CONFIG_AR10_CGU_CLKFSR_VALUE /*store predefined value to CGU_CLKFSR*/
   sw  a0, 0(t2)

   li  t2, PLL0_CFG
   li  a0, CONFIG_AR10_PLL0_CFG_VALUE /*store predefined value to PLL0_CFG*/
   sw  a0, 0(t2)

   li  t2, CGU_UPDATE
   li  a0, 1
   sw  a0, 0(t2) /*write 0x1 to CGU_UPDATE*/
  
   li  t2, RST_REQ
   li  a0, 0x40000000
   sw  a0, 0(t2)  /*issue global software reset*/
wait_reset:
    b   wait_reset
    nop

    .end  cgu_init

			


/*
 * void ddrram_init(long)
 */
  .globl  ddrram_init
  .ent  ddrram_init
ddrram_init:

   .end ddrram_init
   
  .globl  lowlevel_init
  .ent  lowlevel_init
lowlevel_init:
  /* EBU, CGU and SDRAM/DDR-RAM Initialization.
   */
  move  t0, ra

  bal cgu_init
  nop
 
  move  ra, t0
  j ra
  nop

  .end  lowlevel_init
