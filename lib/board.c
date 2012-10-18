#include <common.h>
#include <ddr_setting.h>

#define SRAM_BASE_ADDR     (0x07000000)
#define DDR_BASE_ADDR     (0x050e0000)
#define DDR_DATA_BASE     (0xbffff000)

const char version_string[] =
	"ns115 X-Loader V2.7 (" __DATE__ " - " __TIME__ ")";
xl_header head;
char * pLog;
extern unsigned int busfreq;
extern int flag;

unsigned int check_romloader_fastboot() {
    extern unsigned int _romloader_boot_mode;
    if (_romloader_boot_mode == 0x00002a18) {
        return 1;
    }
    return 0;
}

int is_index_in(int idx)
{
	int i;
	for(i=0;ddr_data[i].offset<=0x378;i++) {
		if(ddr_data[i].offset == idx)
			return 1;
	}
	return 0;
}
void start_armboot (void)
{
 	volatile int i, size = 20;
	unsigned char *buf;
	unsigned int  crc, temp,tmp;
	unsigned int workmode;
	
	board_init();
	uart_init(CFG_UART_BAUD_RATE);
	printf("\n\n%s\n", version_string);

#ifdef CFG_NS115_PBX
	writel(0x50000, 0x10000060);
#endif
	cpu_init();
	ddr_init();
	if(flag == 1) 
		mem_test();
	
	int idx = 0;
	unsigned int sram_dat = 0, ddr_dat = 0, ctrl_dat = 0;
	for(idx = 0;idx < 892;)
	{
//		ctrl_dat = readl(DDR_BASE_ADDR + idx);	//must be notice the workaround 0xc8;
		sram_dat = readl(SRAM_BASE_ADDR + idx);
		writel(sram_dat,DDR_DATA_BASE + idx);
//		ddr_dat = readl(DDR_DATA_BASE + idx);
//		idx = idx + 4;
//		printf("sram_dat = %x;ddr_dat = %x ctrl_dat = 0x%x\n",sram_dat,ddr_dat,ctrl_dat);
//		ddr_dat = readl(0xbffff000 + idx);
//			printf("sram_data = %x;ddr_dat = %x\n",sram_data,ddr_dat);
//		if(ctrl_dat != ddr_dat) {
//			if(is_index_in(idx))
//				printf("idx = 0x%x;====ctrl_dat = %x;ddr_dat = %x\n",idx,ctrl_dat,ddr_dat);
//		}
	
		idx = idx + 4;
	}
	
    
    	if (check_romloader_fastboot()) {
        	TRACE(KERN_INFO, "enter fastboot USB boot\n");
        	usb_boot(0x02);
   	}
    
    
	pLog = (char *)CFG_LOG_BUF;
	switch(readl(CFG_BOOT_MODE) & 0x3)
	{
		case 0x01:
			TRACE(KERN_INFO,"SPI\n");
		//	spi_boot(0x01);
			break;			
			//if spi_boot success, it should never return; 
			//if it returns, we should try SD/MMC, so no break here
	
		case 0x00:
			TRACE(KERN_INFO,"SDIO\n");
			sdmmc_continue_boot(0x0); 	
		//	sdmmc_boot(0x0); 	
//			break;
			//if sdmmc_boot success, it should never return; 
			//if it returns, we should try USB, so no break here
			
		case 0x02:
			TRACE(KERN_INFO,"USB\n");
			usb_boot(0x02);
			break;
			
		case 0x03:
			TRACE(KERN_UART,"UART\n");   
			uart_boot(0x03);
			break;
		default:
			TRACE(KERN_ERROR,"No Boot Source\n");
			break;
	}	
	TRACE(KERN_ERROR,"No U-boot found\n");
	while(1);
}

//#if   defined(CFG_NS2810_PBX) || defined(CFG_NS2810_CHIP)
int check_header(xl_header * head)
{
	TRACE(KERN_DEBUG,"header:offset = 0x%x\n", head->offset);
	TRACE(KERN_DEBUG,"header:entry = 0x%x\n", head->entry);
	TRACE(KERN_DEBUG,"header:size = 0x%x\n", head->size);

	if(CFG_HEAD_TAG != *(unsigned int*)(head->tag))
	{
		TRACE(KERN_ERROR,"No TAG\n");
		return 1;
	}

	if(head->header_cksum != checksum32((void*)head, XL_HEAD_SIZE-sizeof(int)))
	{
                TRACE(KERN_ERROR,"Head cksum err\n");
                return 1;
        }

	return 0;
}

void enter_entry(void* buf, int boot_mode)
{
        TRACE(KERN_INFO, "Booting uboot at 0x%x\n", buf);
        (*(void(*)())buf)(boot_mode);
        while(1);
}
void printbuf(const char *fmt, ...)
{
        va_list args;

        va_start (args, fmt);

        pLog += vsprintf(pLog, fmt, args);
        va_end (args);

	if(((int)pLog - (int)CFG_LOG_BUF) > 3840)
		pLog = CFG_LOG_BUF;	
}

unsigned int checksum32(void *addr, unsigned int size)
{
	unsigned int sum;
	
	sum = 0;
	while(size >= 4) {
		sum  += * (unsigned int *) addr;
		addr += 4;
		size -= 4;
	}
	switch(size) {
		case 3:
			sum += (*(unsigned char *)(2+addr)) << 16;
		case 2:
			sum += (*(unsigned char *)(1+addr)) << 8;
		case 1:
			sum += (*(unsigned char *)(0+addr));
	}
	return sum;
}

void mem_test(void)
{
	unsigned int i,j,val,readback,offset,pattern,test_offset;
	unsigned int anti_pattern,len,temp,num_words;
	unsigned int *addr, *dummy,*start;


	//data line test
	addr = (unsigned int*)0x80000000;
  	dummy = (unsigned int*)0x80001000;
  	val = 0x55555555;
	for(; val != 0; val <<= 1) {
      		*addr  = val;
      		*dummy  = ~val; /* clear the test data off of the bus */
      		readback = *addr;
      		if(readback != val) {
           		printf ("FAILURE (data line):expected %08lx,actual %08lx\n",val, readback);
           		return;
      		}
      		*addr  = ~val;
      		*dummy  = val;
      		readback = *addr;
      		if(readback != ~val) {
        		printf ("FAILURE (data line):Is %08lx, i"
					"should be %08lx\n",readback, ~val);  
			return;
      		}
	}
  
	//address test
	start = (unsigned int*)0x80000000;
	pattern = 0xaaaaaaaa;
	anti_pattern = 0x55555555;
	len = 0x8000000;
	for (offset = 1; offset < len; offset <<= 1){
         	start[offset] = pattern;
  	}

  /*
   * Check for address bits stuck high.
   */
  	test_offset = 0;
  	start[test_offset] = anti_pattern;

  	for (offset = 1; offset < len; offset <<= 1) {
      		temp = start[offset];
      		if (temp != pattern) {
          		printf ("\nFAILURE: Address bit stuck high @ 0x%.8lx:" 
				"expected 0x%.8lx, actual 0x%.8lx\n",(ulong)&start[offset],
								pattern, temp);
			return;
      		}
  	}
  	start[test_offset] = pattern;

        /*
         * Check for addr bits stuck low or shorted.
         */
        for (test_offset = 1; test_offset < len; test_offset <<= 1) 
        {
        	start[test_offset] = anti_pattern;
                for (offset = 1; offset < len; offset <<= 1) {
                        temp = start[offset];
                        if ((temp != pattern) && (offset != test_offset)) {
				printf ("\nFAILURE: Address bit stuck low or shorted @ 0x%.8lx:"
					"expected 0x%.8lx, actual 0x%.8lx\n",(ulong)&start[offset],
									pattern, temp);
				return;
               	        }
             	}
                start[test_offset] = pattern;
	}
								
	num_words = 32;
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
        	start[offset] = pattern;
        }
                
        for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
        	temp = start[offset];
               	if (temp != pattern) {
			printf ("\nFAILURE (read/write) @ 0x%.8lx: expected 0x%.8lx,i"
					"actual 0x%.8lx)\n", (ulong)&start[offset], pattern, temp);
			return;
                }

                anti_pattern = ~pattern;
                start[offset] = anti_pattern;
        }
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
        	anti_pattern = ~pattern;
                temp = start[offset];
                if (temp != anti_pattern) {
                	printf ("\nFAILURE (read/write): @ 0x%.8lx:"
                                "expected 0x%.8lx, actual 0x%.8lx)\n",
                                (ulong)&start[offset], anti_pattern, temp);
                        return ;
                }
                start[offset] = 0;
        }

}

void hang (void)
{
	/* if board_hang() returns, hange here */
#ifdef CFG_PRINTF
	printf("X-Loader hangs\n");
#endif
	for (;;);
}

