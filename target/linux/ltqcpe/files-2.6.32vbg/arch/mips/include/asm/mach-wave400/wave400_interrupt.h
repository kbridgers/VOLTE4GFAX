#ifndef __WAVE400_INTERRUPT_H__
#define __WAVE400_INTERRUPT_H__

#ifdef  MT_GLOBAL
#define MT_EXTERN
#define MT_I(x) x
#else
#define MT_EXTERN extern
#define MT_I(x)
#endif

extern void wave400_enable_irq_all_mips(unsigned long val);

extern void wave400_init_irq(void);

/**
 *      wave400_register_static_irq - register interrupt
 *      @irq_in: Incoming IRQ number for interrupt router
 *      @irq_out: Outgoing IRQ number for interrupt router
 *      @action: IRQ action structure in memory provided by user
 *      @addr: Vector interrupt handler
 *
 * Used to statically register interrupt during system startup.
 */

extern int wave400_register_static_irq(unsigned int irq_in, unsigned int irq_out, 
                                    struct irqaction *action, vi_handler_t addr);

/**
 *      wave400_register_irq - register interrupt
 *      @irq_in: Incoming IRQ number for interrupt router
 *      @irq_out: Outgoing IRQ number for interrupt router
 *      @handler: User's handler function
 *      @flags: IRQ flags
 *      @name: Device name
 *      @dev_id: Owner device ID
 *      @addr: Vector interrupt handler
 *
 * Used to dynamically register interrupt during system operation.
 */

extern int wave400_register_irq(unsigned int irq_in, unsigned int irq_out,
                             irq_handler_t handler, unsigned long flags,
                             const char *name, void *dev_id, vi_handler_t addr);

/**
 *      wave400_unregister_irq - unregister interrupt
 *      @irq_in: Incoming IRQ number for interrupt router
 *      @irq_out: Outgoing IRQ number for interrupt router
 *      @dev_id: Owner device ID
 *
 * Used to dynamically unregister interrupt during system operation.
 */
extern void wave400_unregister_irq(unsigned int irq_in, unsigned int irq_out, void* dev_id);

extern /*static */irqreturn_t uart_interrupt(int irq, void *dev_id);
extern /*static */void wave400_uart_irq(void);
extern /*static */void wave400_net_irq(void);
extern /*static */void wave400_net_g2_irq(void);
extern void wave400_wls_irq(void);
extern void wave400_enable_irq(unsigned int irq_nr);
extern void wave400_disable_irq(unsigned int irq_nr);
extern void wave400_disable_irq_all(void);
extern void wave400_enable_irq_all(void);

//Yan
//#ifdef CONFIG_SYNOPGMACHOST_PCI
//extern irqreturn_t synopGMAC_intr_handler(/*s32*/int intr_num, void * dev_id/*, struct pt_regs *regs*/);
//#endif

#define IRQ_01_NUM	   			0
#define IRQ_02_NUM	   			1
#define IRQ_03_NUM	   			2
#define IRQ_04_NUM	   			3
#define IRQ_05_NUM	   			4
#define IRQ_06_NUM	   			5
#define IRQ_07_NUM	   			0
#define IRQ_08_NUM	   			1
#define IRQ_09_NUM	   			2
#define IRQ_10_NUM	   			3
#define IRQ_11_NUM	   			4
#define IRQ_12_NUM	   			5
#define IRQ_13_NUM	   			0
#define IRQ_14_NUM	   			1
#define IRQ_15_NUM	   			2
#define IRQ_16_NUM	   			3
#define IRQ_17_NUM	   			4
#define IRQ_18_NUM	   			5
#define IRQ_19_NUM	   			0
#define IRQ_20_NUM	   			1
#define IRQ_21_NUM	   			2
#define IRQ_22_NUM	   			3
#define IRQ_23_NUM	   			4
#define IRQ_24_NUM	   			5
#define IRQ_25_NUM	   			0
#define IRQ_26_NUM	   			1
#define IRQ_27_NUM	   			2
#define IRQ_28_NUM	   			3
#define IRQ_29_NUM	   			4
#define IRQ_30_NUM	   			5
#define IRQ_31_NUM	   			0

#define WAVE400_SERIAL_IRQ_IN_INDEX       2
#define WAVE400_TIMER_IRQ_IN_INDEX        28
#define WAVE400_SYNOP_ETHER_IRQ_IN_INDEX  1
#define WAVE400_SYNOP_ETHER_IRQ_GMAC2_IN_INDEX  3
#define WAVE400_WIRELESS_IRQ_IN_INDEX     9

#define WAVE400_SERIAL_IRQ_OUT_INDEX      1
#define WAVE400_TIMER_IRQ_OUT_INDEX       2
#define WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX 3
#ifndef VBG400_NO_ETH_SHARED_IRQ
#define WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX 3
#else
#define WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX 4
#endif
#define WAVE400_WIRELESS_IRQ_OUT_INDEX    5

#undef MT_EXTERN
#undef MT_I

#endif /* __WAVE400_INTERRUPT_H__ */ 
