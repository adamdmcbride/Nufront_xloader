/*  ============================================================================
 */

#include "spi.h"
#include <common.h>

unsigned int temp=0;

int spi_boot(int boot_mode)
{
	int status = 0;
        int sec_cnt = 0;
//        xl_header head;


	spi_init();
		
	SPI_FLASH_read_block(0, XL_HEAD_SIZE,(unsigned char *)&head);	

        status = check_header(&head);
        if(status)
	{
		return status;
	}

	SPI_FLASH_read_block(head.offset,head.size,head.entry);

	if(checksum32(head.entry, head.size) != head.loader_cksum)
        {
        	TRACE(KERN_ERROR, "checksum fail\n");
                return 1;
        }

	enter_entry(head.entry, boot_mode);
}

int spi_init(void)
{
	
	*SPI_0_SSIENR = 0;
	*SPI_0_IMR  = 0x0;	
	*SPI_0_DMACR  = 0x2;
	*SPI_0_DMATDLR = 0x0;


	*SPI_0_BAUDR  = CFG_APB_SPI_CLOCK / CFG_SPI_CLK;		
	
	*SPI_0_CTRLR0  = 0x507;
	*SPI_0_CTRLR1  = 0x0;
	
	
	*SPI_0_SER  = 0x1;	
	*SPI_0_SSIENR = 1;
	
	return 0;	
}

void SPI_FLASH_read_block(unsigned int addr, unsigned int length, unsigned char * buf)
{
	unsigned int value;
	unsigned char temp_addr1,temp_addr2,temp_addr3;
	temp_addr1 = 0;
	temp_addr2 = 0;
	temp_addr3 = 0;


	*SPI_0_SSIENR = 0;
	*SPI_0_CTRLR1  = length - 1;
	*SPI_0_CTRLR0  = 0x3c7;
	*SPI_0_RXFTLR = 0;
	temp_addr1 = addr/65536;
	temp_addr2 = addr/256;
	temp_addr3 = addr%256;
	*SPI_0_SSIENR = 1;
	*SPI_0_DR = 0x3;
	*SPI_0_DR = temp_addr1;
	*SPI_0_DR = temp_addr2;
	*SPI_0_DR = temp_addr3;

	do{
		temp = *SPI_0_SR;
		while((temp & 0x8)==0)
		{
			temp = *SPI_0_SR;
		}
		*buf++ = *SPI_0_DR;

		temp = *SPI_0_SR;
	}while((temp & 0x00000001)==1);
	

		

	return;
}

