/*
 * nufront.com
 */

#ifndef __CONFIG_H
#define __CONFIG_H
#define CURRENT_DEBUG_LEVEL 4 

#define CFG_NS115_PBX
//#define CFG_NS2810_PBX
#define CFG_NS2810_CHIP

#define CFG_HEAD_TAG 'XFUN'

#define CFG_PRINTF 1 
#include <asm/arch/cpu.h>        /* get chip and board defs */
#define SERIAL_BAUD_RATE 2062500
#define RCV_BUF 0x90000000 
#define CFG_LOADADDR 0x80007fc0

//Base & reg
/******************************************************
 **********            PBX	            **********
 *****************************************************/
#ifdef CFG_NS2810_PBX      
#define CFG_APB_CLOCK 25000000

#define UART_0_BASE (0xC5150000)
#define SPI_0_BASE_ADDR (0xC5110000)
#define TIMER_BASE_ADDR (0xC5100000)
#define USB_BASE       (0xC5080000)

#define CFG_APB_TIMER_CLOCK 25000000
#define CFG_APB_UART_CLOCK 25000000
#define SDIO_REGISTERS_OFFSET (0xC5070000)
#define CFG_SDIO_CLOCK 12500
#define CFG_AHB_SDIO_CLOCK 25000


#define CFG_BOOT_MODE   (0x80000000)  
#define CFG_CPU1_ENTRY   0x80000010 
#define CFG_CPU1_CHECK   0x80000014 
#define CFG_CPU0_ENTRY 0x80000008
#define CFG_CPU0_CHECK 0x8000000C  

#define CFG_STACK_ADDR_H  (0xC202)
#define CFG_LOG_BUF    0xC201B000
#define DEV_RX_BUF     0xC201C000
#define DEV_TX_BUF     0xC201D000
#endif

/******************************************************
 **********            CHIP	            **********
 *****************************************************/
#ifdef CFG_NS2810_CHIP 
#define CFG_APB_CLOCK 66000000

#define CFG_APB_TIMER_CLOCK 12300000
//#define CFG_APB_UART_CLOCK  66000000

#define UART_0_BASE     (0x06090000)
#define SPI_0_BASE_ADDR (0x060D0000)
#define TIMER_BASE_ADDR (0x06150000)
#define USB_BASE        (0x050A0000)
#define SDIO_REGISTERS_OFFSET (0x05070000)
#define CFG_AHB_SDIO_CLOCK 25000      //?????????????
#define CFG_SDIO_CLOCK 12500	      //???????????

#define CFG_BOOT_MODE   (0x0582207C)  
#define CFG_CPU0_ENTRY  (0x05821020)
#define CFG_CPU0_CHECK  (0x05821024)
#define CFG_CPU1_ENTRY  (0x05821028) 
#define CFG_CPU1_CHECK  (0x0582102C)
#define CFG_STACK_ADDR_H  (0x0705)
#define CFG_LOG_BUF    	(0x0704B000)
#define DEV_RX_BUF      (0x9f900000)
#define DEV_TX_BUF      (0x0704D000)
#define DDR3_800    
#define CFG_UART_BAUD_RATE    2062500//115200
#endif

//memory config
//#define CFG_IRAM_SIZE	0x00020000  //128KB

//SCM & PRCM
//X-loader
#define CFG_XLOADER_ADDR 	4160  //64 sector from 0

// serial defines 
//#define CFG_UART_BAUD_RATE    2062500//115200
#define CFG_PBSIZE 256

//spi defines
#define CFG_SPI_CLK 1000000

#endif /* __CONFIG_H */

