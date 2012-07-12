#ifndef _SPI_INCLUDE_
#define _SPI_INCLUDE_

//MASTER CONFIG

#define SPI_0_CTRLR0	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x00))
#define SPI_0_CTRLR1	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x04))
#define SPI_0_SSIENR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x08))
#define SPI_0_MWCR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x0C))

#define SPI_0_SER		((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x10))
#define SPI_0_BAUDR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x14))
#define SPI_0_TXFTLR  ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x18))
#define SPI_0_RXFTLR  ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x1C))

#define SPI_0_TXFLR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x20))
#define SPI_0_RXFLR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x24))
#define SPI_0_SR		((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x28))
#define SPI_0_IMR		((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x2C))

#define SPI_0_ISR		((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x30))
#define SPI_0_RISR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x34))
#define SPI_0_TXOICR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x38))
#define SPI_0_RXOICR  ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x3C))

#define SPI_0_RXUICR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x40))
#define SPI_0_MSTICR  ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x44))
#define SPI_0_ICR		((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x48))
#define SPI_0_DMACR	((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x4C))

#define SPI_0_DMATDLR ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x50))
#define SPI_0_DMARDLR ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x54))
#define SPI_0_IDR		((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x58))
#define SPI_0_SSI_COMP_VERSION ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x5C))

#define SPI_0_DR ((volatile unsigned int*)(SPI_0_BASE_ADDR + 0x60))  //0X60 - 0X9C 
#endif
