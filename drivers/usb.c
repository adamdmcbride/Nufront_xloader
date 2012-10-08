
#include "usb.h"
#include <common.h>

#define GET_STATUS 0
#define SET_ADDRESS 5
#define GET_DESCRIPTOR 6
#define SET_CONFIGURATION 9


#define MASK_ADDR  0xfffff000
#define SETID_ADDR 0x05800000

#define DESCRIPTOR_DEVICE 1
#define DESCRIPTOR_CONFIGURATION 2
#define DESCRIPTOR_STRING 3

//typedef unsigned int   uint32_t;
//typedef unsigned char  uint8_t;

#define app_write_single(reg,val)  (*(volatile uint32_t *)(reg) = (val))
#define app_read_single(reg)  (*(volatile uint32_t *)(reg))
#define usb_readl(offset)       (*(volatile uint32_t *)(USB_BASE + (offset)))
#define usb_writel(offset,val)  (*(volatile uint32_t *)(USB_BASE + (offset)) = (val))

#define USB_MPS 64

void go_error(const char *error) { 
	TRACE(KERN_DEBUG,"go error !!!!!!!!!!!! %s ", error);
	for(;;) {} 
}
#define assert(x)  if ( !(x) ) go_error(#x);
void wait_intr(uint32_t offset, uint32_t val);
void wait_cycle(int n);
volatile int dlflag = 0;
volatile int dlsize = 0;
volatile char * rxdata = 0;

//------------------------------------------------------------//

const unsigned char device_config[20] __attribute__ ((aligned (4)))=
{
	0x12,
	0x01,
	0x00,//0x01,
	0x02,

	0x00,
	0x00,
	0x00,
	0x40,

	0xB4,
	0x0B,
	0xFF,
	0x0F,

	0x00,
	0x01,
	0x01,
	0x01,

	0x01,
	0x01,
	0xAA,
	0x55,
};

const unsigned char config_descriptor[32] __attribute__ ((aligned (4)))=
{
	/* Configuration Descriptor */
	0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0x80, 0x80,

	/* Interface Descriptor */
	0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x42, 0x03, 0x00,

	/* Endpoint Descriptor */
	0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
	0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x01,

};

const unsigned char zero[8] __attribute__ ((aligned (4))) = 
{
	0x00, 0x00, 0x00, 0x00,
};

const unsigned char language_descriptor[8] __attribute__ ((aligned (4)))=
{
	0x04, 0x03, 0x09, 0x04,
};

static volatile unsigned char string_descriptor[256] __attribute__ ((aligned (4))) =
{ 
	//0x12, 0x03, 'x', 0x00, 'l', 0x00, 'o', 0x00, 'a', 0x00, 'd', 0x00, 'e', 0x00, 'r', 0x00, '1', 0x00,
    0x4, 0x03, '?', 0x0,
};

void init_serial() {
	extern volatile unsigned char _serialno;
	unsigned char*  pSerial = &_serialno;
	unsigned char*  pDest = string_descriptor;
	int i = 0;
	for (i = 0; i < (int)_serialno; i++) {
		*pDest++ = *pSerial++;
	}
	//printf("serial_size:%d,%d", (int)string_descriptor[0], i);
}

void usb_dev_init(void)
{
	init_serial();

	int i;

	// Init Step 1 : Programming the Core (6.1)
	// 1.1 Read hardware configuration
	//TRACE(KERN_DEBUG,"GSNPSID = 0x%08X\n", usb_readl(GSNPSID));
	//TRACE(KERN_DEBUG,"GHWCFG1 = 0x%08X\n", usb_readl(GHWCFG1));
	//TRACE(KERN_DEBUG,"GHWCFG2 = 0x%08X\n", usb_readl(GHWCFG2));
	//TRACE(KERN_DEBUG,"GHWCFG3 = 0x%08X\n", usb_readl(GHWCFG3));
	//TRACE(KERN_DEBUG,"GHWCFG4 = 0x%08X\n", usb_readl(GHWCFG4));
	//TRACE(KERN_DEBUG,"GLPMCFG = 0x%08X\n", usb_readl(GLPMCFG));

	// 1.2 Set AHB configuration
	// Core in slave Mode
	// Unmask the interrupt assertion to the application
	// Non-Periodic TxFIFO Empty Level: completely empty
	// Periodic TxFIFO Empty Level: completely empty
	usb_writel(GAHBCFG, 0x181);

	// 1.3 Enable interrupts:
	// GINTMSK.RxFLvlMsk = 1'b0
	// usb_writel(GINTMSK, 0);  //default is 0

	// 1.4 Set GUSBCFG:
	//  480-MHz Internal PLL clock
	//  The MAC interface is 16-bit UTMI+
	//  HNP capability is not enabled
	//  SRP capability is not enabled
	//  Single Data Rate ULPI Interface, with 8-bit-wide data bus
	//  USB 2.0 high-speed UTMI+ or ULPI PHY
	//  6-pin unidirectional full-speed serial interface
	//  UTMI+ Interface
	//  PHY Interface: 16-bits
	usb_writel(GUSBCFG, 0x3C00);

	// 1.5 Enable interrupts:
	// OTG Interrupt Mask
	// Mode Mismatch Interrupt Mask
	usb_writel(GINTMSK, 0x6);  

	// Init step 2: Device Init (6.3.3)
	// 2.1 Set DCFG:
	//  Resume Validation Period: 2
	//  IN Endpoint Mismatch Count: 4
	// + Periodic Frame Interval: 1 (85%)
	usb_writel(DCFG, 0x08100801);
        wait_cycle(50);
	// 2.2 Enable thresholding
	// 
	
	// 2.3 Enable interrupts:
	//  OTG Interrupt
	//  Mode Mismatch Interrupt
	//  Start of (micro)Frame Mask 
	//  Early Suspend Mask
	//  USB Suspend Mask
	//  USB Reset Mask
	//  Enumeration Done Mask
	usb_writel(GINTMSK, 0x3c0e);
	
	// + Data line pulsing using utmi_termsel 
	//usb_writel(GUSBCFG, 0x401408);
	// clear soft disconnect
	usb_writel(DCTL, 0x0);
	wait_cycle(10);


	usb_writel(GRSTCTL, (0x10<<6)|(1<<5)|(1<<4));
	// 2.4 wait for USB reset
	wait_intr(GINTSTS, 0x1000);
	TRACE(KERN_DEBUG,"usb reset ...");

	// Initialization on USB Reset (6.6.1.1)

	// 2.4.1 Set the NAK bit for all OUT endpoints
	//usb_writel(DOEPCTL0, 1<<27);

	// 2.4.2Enable interrupts for IN/OUT EP 0
	usb_writel(DAINTMSK, 0x10001);
	// Enable Device OUT Endpoint Common Interrupts:
	//  SETUP Phase Done
	//  Endpoint Disabled Interrupt
	//  Transfer Completed Interrupt
	usb_writel(DOEPMSK, 0xb);
	// Enable Device IN Endpoint Common Interrupts:
	//  IN Endpoint NAK Effective
	//  Timeout Condition
	//  Endpoint Disabled Interrupt Mask
	//  Transfer Completed Interrupt Mask
	usb_writel(DIEPMSK, 0x4b);

	// 2.4.3 Set for receive data 
	// mask GINTMSK.NPTxFEmpMsk, and GINTMSK.RxFLvlMsk
	//usb_writel(GINTMSK, 0x3c3e);
	usb_writel(GINTMSK, 0x3c8e);

	// 2.4.4 Program the GRXFSIZ Register, to be able to receive control OUT data and setup data.

	// 2.4.5 
	usb_writel(DOEPTSIZ0, 0x00180018);
		// 2.5 
	wait_intr(GINTSTS, 0x2000);
	TRACE(KERN_DEBUG,"Enum...\n");
	
	usb_writel(DIEPCTL0, 0);
	usb_writel(DOEPCTL0, 0x80000000);
	

	//Reset FIFOs
	// Waiting for Enumeration Done

	
	usb_writel(GINTMSK, 0xfb0c3c0c);
	usb_writel(GINTSTS, 0xffffffff);

	usb_writel(DIEPMSK,  0xffffffff);
	usb_writel(DOEPMSK,  0xffffffff);
	usb_writel(DAINTMSK, 0xffffffff);

	// Device OUT Endpoint 0 Transfer Size Register
	//  Packet Count: 3
	//  Transfer Size: 24
	//usb_writel(DOEPTSIZ0, 0x0);
	usb_writel(DOEPTSIZ0, 0x60180018);
	//usb_writel(DOEPDMA0, TEST_BUF_ADDR);
	usb_writel(DOEPCTL0, 0x84000000);
	//TRACE(KERN_DEBUG,"Waiting for Enumeration Done ");


}


void Setup_Tx(unsigned int *data, unsigned int len)
{
	int i;
	int wlen = (len+3) / 4;
	volatile int val = 0;

	//reset tx fifo
	usb_writel(GRSTCTL, (0x10<<6)|(1<<5));

	//usb_writel(DIEPINT0, 0xFFFFFFFF);
	//usb_writel(DOEPINT0, 0xFFFFFFFF);

	usb_writel(DIEPTSIZ0, 0x80000|len);
	do{
		val = usb_readl(GNPTXSTS);
	
		//at least 1 queue location available	
		if((val & 0x01FF0000) == 0x0)
			continue;

		if(wlen==0)
			break;
		
		//if non-zero-length packet, should have enough FIFO space	
		if((val & 0xFFFF) >= wlen)
			break;
		
	}while(1);

	val = usb_readl(DIEPCTL0);
	usb_writel(DIEPCTL0, val | 0x80000000);
	
	val = usb_readl(DIEPINT0);
	if(val &  0x00000040)
		usb_writel(DIEPINT0, val | 0x00000040);
	
	val = usb_readl(DIEPCTL0);
	usb_writel(DIEPCTL0, val | 0x84000000);

	wait_intr(GINTSTS, 0x20);
	usb_writel(GINTSTS, 0x20);


	for(i=0; i<wlen; i++)
	{
		usb_writel(DEVEP0_FF, data[i]);
		//printf("%x,(%d,%d) ", data[i], i,wlen);
	}
	//printf("\n");
//	wait_cycle(100);
	do{
		val = usb_readl(DIEPINT0);
	
		if(val & 0x1) //xfer complete
		{
			usb_writel(DIEPINT0,0x1);
			break;		
		}

	}		
	while(1);
	
	return;
}

void Read_Fifo(unsigned int * buf, unsigned int len)
{
	int i;
	int wlen = (len+3)/4;
	for(i=0; i<wlen; i++)
		buf[i] = usb_readl(DEVEP0_FF);
	return;
}

void Setup_Ack()
{
	// Device IN Endpoint 0 Transfer Size Register
	//   Packet Count: 1
	usb_writel(DIEPINT0, 0xFFFFFFFF);
	usb_writel(DOEPINT0, 0xFFFFFFFF);
	usb_writel(DIEPTSIZ0, 0x80000);
	//b_writel(DIEPDMA0, TEST_BUF_ADDR);
	// Device Control IN Endpoint 0 Control Register
	//   Endpoint Enable
	//   Next Endpoint 1
	usb_writel(DIEPCTL0, 0x84000000);
	wait_intr(DIEPINT0, 1);
	usb_writel(DIEPINT0, 1);
}


static unsigned int PID_DATA;
void active_bulk_data(int size)
{
	int pkcnt = (size + USB_MPS - 1)/USB_MPS;

	if(PID_DATA == 1) {
		PID_DATA = 2;
	} else {
		PID_DATA = 1;
	}

	// Device OUT Endpoint 1 DMA Address Register
	usb_writel(DOEPTSIZ1, pkcnt << 19 | size);
	// Device Control OUT Endpoint 1 Control Register
	//   Maximum Packet Size: USB_MPS
	//   Next Endpoint: 1
	//   Endpoint Type: BULK
	//   Clear NAK
	//   Set DATA0 PID
	//   Endpoint Enable
	usb_writel(DOEPCTL1, 0x84088000|(PID_DATA<<28)|USB_MPS);

	usb_writel(DIEPCTL2, 0x08088000|USB_MPS);
	
}

void bulk_in(unsigned int *data, unsigned int len)
{
	int i;
	int wlen = (len+3) / 4;
	volatile int val = 0;

	//reset tx fifo
	usb_writel(GRSTCTL, (0x10<<6)|(1<<5));

	//EP2 is bulk IN
	usb_writel(DIEPTSIZ2, 0x80000|len);
	do{
		val = usb_readl(GNPTXSTS);
	
		//at least 1 queue location available	
		if((val & 0x01FF0000) == 0x0)
			continue;

		if(wlen==0)
			break;
		
		//if non-zero-length packet, should have enough FIFO space	
		if((val & 0xFFFF) >= wlen)
			break;
		
	}while(1);

	val = usb_readl(DIEPCTL2);
	usb_writel(DIEPCTL2, val | 0x80000000);
	
	val = usb_readl(DIEPINT2);
	if(val &  0x00000040)
		usb_writel(DIEPINT2, val | 0x00000040);
	
	val = usb_readl(DIEPCTL2);
	usb_writel(DIEPCTL2, val | 0x84000000);

	wait_intr(GINTSTS, 0x20);
	usb_writel(GINTSTS, 0x20);


	for(i=0; i<wlen; i++)
	{
		usb_writel(DEVEP2_FF, data[i]);
	}
	do{
		val = usb_readl(DIEPINT2);
		
		if(val & 0x1) //xfer complete
		{
			usb_writel(DIEPINT2,0x1);
			break;		
		}

	}		
	while(1);
	
	return;
}

void handle_fastboot(unsigned char * buf, unsigned int len, int boot_mode)
{
	char * txbuf = (char*)DEV_TX_BUF;
	int addr = 0;
	unsigned int temp;

	if(dlflag == 3)  //the first packet contains header	
	{
		memcpy(&head, buf, sizeof(xl_header));
		dlflag = 2;
		dlsize -= len;
	}
	else if(dlflag == 2)
	{
		dlsize -= len;		
			
		if(dlsize == head.size) //binary data comes
		{
			dlflag = 1;
			rxdata = head.entry;
		}
	}
	else if(dlflag == 1)
	{
		dlsize -= len;
		rxdata += len;
		strncpy(txbuf,"OKAY",4);
		if(dlsize == 0) //data download finish
		{
			dlflag = 0;
			if((check_header(&head)) || (checksum32(head.entry, head.size) != head.loader_cksum))
			{
				strncpy(txbuf,"FAILCKSUMERR",12);
				bulk_in((unsigned int*)txbuf,12);
			}
			else	
			bulk_in((unsigned int*)txbuf,4);
		}

	}
	else if(!strncmp(buf,"getvar:",7))
	{
		if(!strncmp(buf+7,"version",7))
		{
			strcpy(txbuf,"OKAYNS281000");
			
		}
		if(!strncmp(buf+7,"0x",2))
		{
			addr = simple_strtoul(buf+7,NULL,16);	
			sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		}
			bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"download:",9))
	{
		strncpy(txbuf,"DATA",4);
		strncpy(txbuf+4,buf+9,8);
		
		buf += 7;
		buf[0] = '0';
		buf[1] = 'x';
		buf[10]= NULL;
	
		dlsize = simple_strtoul(buf,NULL,16);
		dlflag = 3;
		bulk_in((unsigned int*)txbuf,12);
		
	}
	else if(!strncmp(buf,"setid1",6))
	{	
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x1;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid2",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x2;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid3",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x3;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid4",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x4;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid5",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x5;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid6",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x6;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid7",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x7;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"setid8",6))
	{
		temp = readl(SETID_ADDR);
		temp = (temp & MASK_ADDR) | 0x8;
		writel(temp,SETID_ADDR);
		addr = 0x95800000;
		sprintf(txbuf,"OKAY%8x", *(unsigned int*)addr);
		bulk_in((unsigned int*)txbuf,12);
	}
	else if(!strncmp(buf,"boot",4))
	{
		strncpy(txbuf,"OKAY",4);	
		bulk_in((unsigned int*)txbuf,4);
		enter_entry(head.entry, boot_mode);
	}
}

void handle_setup(unsigned char *rev_buf)
{
    TRACE(KERN_DEBUG,"SETUP:%x\n",rev_buf[0]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[1]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[2]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[3]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[4]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[5]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[6]);
    TRACE(KERN_DEBUG,"%x\n",rev_buf[7]);

	switch (rev_buf[1]) {
	case GET_DESCRIPTOR:
		switch (rev_buf[3]) {
		case DESCRIPTOR_DEVICE:
			Setup_Tx(device_config, 18);
			TRACE(KERN_DEBUG,"IN Dev desc\n");
			break;
		case DESCRIPTOR_CONFIGURATION:
			if (rev_buf[6] == 0x09) {
				Setup_Tx(config_descriptor, 0x09);
			} else {
				Setup_Tx(config_descriptor, 0x20);
			}
			TRACE(KERN_DEBUG,"In Cfg desc\n");
			break;
		case DESCRIPTOR_STRING:
			if (rev_buf[2] == 0x00) {
				Setup_Tx(language_descriptor, 0x04);
			} else {
				//printf("query_serial:\n");
				Setup_Tx((unsigned int*)string_descriptor, (unsigned int)string_descriptor[0]);
			}
			TRACE(KERN_DEBUG,"In lang desc\n");
			break;
		default:
			TRACE(KERN_DEBUG,"other desc %d\n", rev_buf[3]);
			break;
		}
		break;
	case SET_ADDRESS:
		TRACE(KERN_DEBUG,"Set Address %d\n", rev_buf[2]);
		usb_writel(DCFG, 0x08100800|(rev_buf[2]<<4));
		Setup_Tx(zero,0);
		break;
	case SET_CONFIGURATION:
		TRACE(KERN_DEBUG,"Set Configuration\n");
		Setup_Tx(zero,0);
		active_bulk_data(USB_MPS);

		//test
		#if 0
		TRACE(KERN_DEBUG,"wait in command");
		while(1){
			wait_bulk_in();
			TRACE(KERN_DEBUG,"have in");
		}
		while(1) 
		#endif
		break;
	case GET_STATUS:
		TRACE(KERN_DEBUG,"Get Status\n");
		Setup_Tx(zero, 0x02);
		break;
	default:
		TRACE(KERN_DEBUG,"Unknown Setup packet %d\n", rev_buf[1]);
		break;
	}
}


int usb_boot(int boot_mode)
{
	int i;
	unsigned int intvalue = 0;
	unsigned int intmask = ~0x0404B428;
	unsigned int v;
	unsigned int rxsize = 0;
	volatile unsigned int rxstat = 0;

	TRACE(KERN_DEBUG,"RESET USB ... ");
	usb_writel(DCTL, 0x02);
	wait_cycle(10000000);
	usb_writel(GRSTCTL, 0x01);
	while( (usb_readl(GRSTCTL) & 0x01) != 0x00 )
	{
	        wait_cycle(1);
	}
	TRACE(KERN_DEBUG,"done!\n");
	
	usb_dev_init();
	PID_DATA = 2;
	rxdata = DEV_RX_BUF;

	while(1) {
		while((intvalue = (usb_readl(GINTSTS)&intmask)) == 0);
		if(intvalue & (1<<4))   //Rx fifo not empty
		{
			rxstat = usb_readl(GRXSTSP);  //Read receive status register
			rxsize = (rxstat & 0x00007FF0) >> 4;			
			rxstat &= 0x001E000F;

			if(rxstat == 0x000C0000)  //Setup packet, DATA0, EP 0
			{
				Read_Fifo(DEV_RX_BUF, rxsize);
				handle_setup((unsigned char *)DEV_RX_BUF);
				usb_writel(DOEPTSIZ0, 0x60180018);
				usb_writel(DOEPCTL0, 0x84000000);
				usb_writel(GINTSTS,1<<4);  //Clear
			}
			else if(rxstat == 0x00040001)  //OUT packet on EP 1
			{
				usb_writel(DOEPCTL1, usb_readl(DOEPCTL1) | 0x08000000); //set NAK
				Read_Fifo(rxdata, rxsize);
				handle_fastboot(rxdata, rxsize, boot_mode);
				
				active_bulk_data(USB_MPS);	

			}

		}

	}

}

void wait_intr(uint32_t offset, uint32_t val)
{
	while( (usb_readl(offset) & val) != val )
	{
		wait_cycle(1);
	}
}

void wait_cycle(int n)
{
	while(n-- > 0);
}
