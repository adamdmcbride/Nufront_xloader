/*
 * (C) Copyright 2004
 * Texas Instruments
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

#undef	_LINUX_CONFIG_H
#define _LINUX_CONFIG_H 1	/* avoid reading Linux autoconf.h file	*/

typedef unsigned char		uchar;
typedef volatile unsigned long	vu_long;
typedef volatile unsigned short vu_short;
typedef volatile unsigned char	vu_char;

#include <config.h>
#include <linux/types.h>
#include <stdarg.h>

#ifdef CONFIG_ARM
#define asmlinkage	/* nothing */
#endif

#define NULL (0)
#define readl(a)       (*((volatile unsigned int *)(a)))
#define writel(v,a)    (*((volatile unsigned int *)(a)) = (v))

//#define REG_WRITE(addr, value)  (*((volatile unsigned int*)(addr))) = (value)
//#define REG_READ(addr)  (*((volatile unsigned int*)(addr)))

typedef struct {
	char         tag[4];    //NUFX
	unsigned int offset;    //
	unsigned int entry;     //where to place 
	unsigned int size;       //size of binary 
	unsigned int loader_cksum; //chsum of binary
	unsigned int header_cksum; //cksum of first 16 bytes of header
	}xl_header;
extern xl_header head;
#define XL_HEAD_SIZE (sizeof(xl_header))

#if 0
//***********************  Add the ns2810_pbx *********************//

#define printf(fmt,args...)     serial_printf (fmt ,##args)
#define getc() uart_getc()
#define KERN_UART     1
#define KERN_ERROR    2
#define KERN_INFO     3
#define KERN_DEBUG    4
#define TRACE(level,fmt,args...)  ( ((level) <= CURRENT_DEBUG_LEVEL ) ? printf(fmt,##args) : printbuf(fm    t,##args))

///////////////////////////////////////////////////////////////////////

#endif

#ifdef CONFIG_ARM
# include <asm/setup.h>
# include <asm/x-load-arm.h>	/* ARM version to be fixed! */
#endif /* CONFIG_ARM */

#ifdef	CFG_PRINTF
#define printf(fmt,args...)	serial_printf (fmt ,##args)
#define getc() uart_getc()
#define KERN_UART     1	
#define KERN_ERROR    2
#define KERN_INFO     3
#define KERN_DEBUG    4
#define TRACE(level,fmt,args...)  ( ((level) <= CURRENT_DEBUG_LEVEL ) ? printf(fmt,##args) : printbuf(fmt,##args))
#define debug(fmt,args...)
#else
#define printf(fmt,args...)
#define TRACE(fmt,args...)
#define getc() ' '
#endif	/* CFG_PRINTF */

/* board/$(BOARD)/$(BOARD).c */
int 	board_init (void);
unsigned int checksum32(void *addr, unsigned int size);
void enter_entry(void* buf, int boot_mode);

/* cpu/$(CPU)/cpu.c */
int 	cpu_init (void);

#ifdef CFG_PRINTF

/* serial driver */
int uart_init(int baudrate);
void	uart_setbrg (void);
void	uart_putc   (const char);
void	uart_puts   (const char *);
int	uart_getc   (void);
int	uart_tstc   (void);

/* lib/printf.c */
void	serial_printf (const char *fmt, ...);

//void    serial_printf (const char *fmt, ...);
void    printbuf (const char *fmt, ...);
int     sprintf (char* buf, const char *fmt, ...);
#endif

unsigned int crc32 (uint32_t crc, const char* buf, unsigned int len);
int spi_0_init(void);
unsigned char * i2c_master_init(unsigned char slv_addr);

/* driver/nusmart_mmc.c */
int     sdmmc_boot(int boot_mode);


/* driver/uart.c */
#define E_OK        ('O' + 'K' * 256)
#define E_CHECKSUM  ('E' + '1' * 256)
#define E_BADCMD    ('E' + '2' * 256)
#define E_TIMEOUT   ('E' + '3' * 256)
#define E_DEBUG     ('D' + 'B' * 256)

#define XCMD_CONNECT      ('H' + 'I' * 256)
#define XCMD_SET_BAUD     ('S' + 'B' * 256)
#define XCMD_DOWNLOAD     ('D' + 'L' * 256)
#define XCMD_UPLOAD       ('U' + 'L' * 256)
#define XCMD_SOC_MEMSET   ('M' + 'S' * 256)
#define XCMD_SOC_CALL     ('B' + 'L' * 256)


/* driver/spi.c */
int spi_0_init(void);
int spi_boot(int boot_mode);

/* driver/timer.c */
void u_delay(unsigned int us);

/* driver/usb.c */
int usb_boot(int boot_mode);

/* driver/ddr/ddr_init.c*/
void ddr_init(void);
void ddr_sel(void);

/* lib/board.c */
void	hang		(void) __attribute__ ((noreturn));
#endif	/* __COMMON_H_ */
