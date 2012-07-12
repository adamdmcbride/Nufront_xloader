/*
 * nufront.com
 */

#include <asm/arch/cpu.h>        /* get chip and board defs */
#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_NS115_FPGA

#define CURRENT_DEBUG_LEVEL 3 

#define CFG_HEAD_TAG 'XFUN'

#define CFG_PRINTF 1 

//Base & reg
#define CFG_APB_UART_CLOCK 24000000
#define CFG_APB_TIMER_CLOCK 24000000
#define UART_0_BASE     (0x06090000)
#define SPI_0_BASE_ADDR (0x060D0000)
#define TIMER_BASE_ADDR (0x06150000)
#define USB_BASE        (0x050A0000)
#define SDIO_REGISTERS_OFFSET (0x05070000)
#define CFG_AHB_SDIO_CLOCK 24000      //?????????????
#define CFG_SDIO_CLOCK 6250	      //???????????

#define CFG_BOOT_MODE   (0x0582207C)  
#define CFG_CPU0_ENTRY  (0x05821020)
#define CFG_CPU0_CHECK  (0x05821024)
#define CFG_CPU1_ENTRY  (0x05821028) 
#define CFG_CPU1_CHECK  (0x0582102C)
#define CFG_STACK_ADDR_H  (0x0702)
#define CFG_LOG_BUF    	(0x0701B000)
#define DEV_RX_BUF      (0x8f900000)
#define DEV_TX_BUF      (0x0701D000)
#define DDR3_800    
#define CFG_UART_BAUD_RATE    115200
#define CFG_STACK_ADDR  (0x05801000)

//SCM & PRCM
//X-loader
//#define CFG_XLOADER_ADDR 	4608  //64 sector from 0

#define CFG_UBOOT_ADDR  4608
// serial defines 
#define CFG_PBSIZE 256

//spi defines
#define CFG_SPI_CLK 1000000

#endif /* __CONFIG_H */

