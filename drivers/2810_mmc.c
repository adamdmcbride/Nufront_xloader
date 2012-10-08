/*
 * The driver for Evatronix SDIO Host Controller.
 *
 * Author: Pan Ruochen <ruochen.pan@nufront.com>
 *
 * 2011 (c) Nufront, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
//#include "basetype.h"
//#include "mach-nusmart/io.h"
//#include "nufront_rom_def.h"

#include <common.h>

#include "mmc.h"
#include "mmc_host_def.h"
#include "SDIO_Host.h"
#include "SDIO_CardGeneral.h"

typedef unsigned long lbaint_t;

struct sdio_slot {
	volatile struct slot_register_set *io;
	unsigned char  id;
	unsigned char  DeviceType;
	unsigned char  inserted;
	unsigned char  SupportedBusWidths, SpecVersNumb;
	unsigned char  DeviceCapacity, IsSelected;
	unsigned short RCA;
	unsigned int cmd;
	unsigned int response[4];

	unsigned short blkcnt;
	unsigned short blksize;
};

#define SDMMC_SECTOR_SIZE    512

//static unsigned char HostBusMode;

static int WaitForValue(volatile unsigned int *addr, unsigned int mask, int is_set, unsigned int timeout)
{
	if ( is_set == 0 ){
		// wait until bit/bits will clear
		while ( timeout-- > 0 && ( *addr & mask ) != 0) {}
	} else {
		// wait until a bit/bits will set
		while ( timeout-- > 0 && ( *addr & mask ) == 0) {}
	}
	return timeout == 0 ? SDIO_ERR_TIMEOUT : SDIO_ERR_NO_ERROR;
}

static inline int check_cmd_error(struct sdio_slot *slot)
{
	static const unsigned char error_code[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		SDIO_ERR_COMMAND_TIMEOUT_ERROR,
		SDIO_ERR_COMMAND_CRC_ERROR,
		SDIO_ERR_COMMAND_END_BIT_ERROR,
		SDIO_ERR_COMMAND_INDEX_ERROR,
		SDIO_ERR_DATA_TIMEOUT_ERROR,
		SDIO_ERR_DATA_CRC_ERROR,
		SDIO_ERR_DATA_END_BIT_ERROR,
		SDIO_ERR_CURRENT_LIMIT_ERROR,
		SIO_ERR_AUTOCMD12_NOT_EXECUTED,
		SDIO_ERR_ADMA,
		SDIO_ERR_TUNING,
	};
	unsigned long val;
	int i;

	val = slot->io->SRS12;
	if( val == 0 )
		return 0;
	asm("clz %0,%1\n\t" : "=r"(i) : "r"(val) );
	return error_code[31 - i];
	/*
	   switch(31 - i) {
	   case 16:  return SDIO_ERR_COMMAND_TIMEOUT_ERROR;
	   case 17:  return SDIO_ERR_COMMAND_CRC_ERROR;
	   case 18:  return SDIO_ERR_COMMAND_END_BIT_ERROR;
	   case 19:  return SDIO_ERR_COMMAND_INDEX_ERROR;
	   case 20:  return SDIO_ERR_DATA_TIMEOUT_ERROR;
	   case 21:  return SDIO_ERR_DATA_CRC_ERROR;
	   case 22:  return SDIO_ERR_DATA_END_BIT_ERROR;
	   case 23:  return SDIO_ERR_CURRENT_LIMIT_ERROR;
	   case 24:  return SIO_ERR_AUTOCMD12_NOT_EXECUTED;
	   case 25:  return SDIO_ERR_ADMA;
	   case 26:  return SDIO_ERR_TUNING;
	   }
	   return 0;
	 */
}

/*
 * function: send one command to CID, which need wait CD bit of CMD register
 * parameter:
 *              cmd -- command, write it to CMD regiseter
 *      arg -- command argument, write it to CMDARG register    
 *      response -- responses of the current command will write to it
 */
static int SDIOHost_ExecCardCommandTimeout(struct sdio_slot *slot, unsigned int cmd, unsigned int arg, unsigned int timeout)
{
	//volatile unsigned int mmc_stat;
	int status;

	/* Clear all interrupt status */
	slot->io->SRS12 = 0xffffffff;

	TRACE(KERN_DEBUG, "CMD%u: 0x%08x, ARG: 0x%08x\n", (cmd>>24)%64, cmd, arg);

	slot->cmd = cmd;
	if(cmd & __CMD_DATA_PRESENT) {
		slot->io->SRS1 = (slot->blkcnt << 16) | slot->blksize | SFR1_DMA_BUFF_SIZE_512KB;
		if(slot->blkcnt > 1)
			cmd |= 0x22; /* Enable Multi Block, Block Count */
		TRACE(KERN_DEBUG, "BlockCount=%u, BlockSize=%u\n", slot->blkcnt, slot->blksize);
	}

	while( slot->io->SRS9 & 1 ) {}
	slot->io->SRS2 = arg;
	slot->io->SRS3 = cmd;
	while (1) {
		status = WaitForValue(&slot->io->SRS9,  1, 0, timeout);
		if(status != 0)
			return status;
		status = WaitForValue(&slot->io->SRS12, 1, 1, timeout);
		if(status != 0)
			return status;

		status = check_cmd_error(slot);
		if( status != 0 ) {
			TRACE(KERN_DEBUG, "status: %d\n", status);
			return status;
		}

		if ( RESPONSE_48BIT(cmd) )
		{
			slot->response[0] = slot->io->SRS4;
			TRACE(KERN_DEBUG,"Resp48:0x%x\n",slot->response[0]);
		}

		//long responses
		if ( RESPONSE_136BIT(cmd) ) {
			slot->response[1] = slot->io->SRS5;
			slot->response[2] = slot->io->SRS6;
			slot->response[3] = slot->io->SRS7;
			TRACE(KERN_DEBUG,"Resp136:0x%x 0x%x 0x%x\n",slot->response[1],slot->response[2],slot->response[3]);
		}
		break;
	}
	return SDIO_ERR_NO_ERROR;
}
#define SDIOHost_ExecCardCommand(slot, cmd, arg)  SDIOHost_ExecCardCommandTimeout(slot, cmd, arg, 0xFFFFFFFF)

static int SDIOHost_ExecCMD55Command(struct sdio_slot *slot)
{
	return SDIOHost_ExecCardCommand(slot, SDMMC_CMD55, slot->RCA << 16);
}


static int SDIOHost_ReadDataBlock(struct sdio_slot *slot, void *buf)
{
	unsigned int i, blkcnt;
	int status;

	blkcnt = slot->blkcnt;
	while(blkcnt > 0) {
		status = WaitForValue(&slot->io->SRS12, (SFR12_BUFFER_READ_READY | SFR12_ERROR_INTERRUPT),
				1, COMMANDS_TIMEOUT );
		if(status || (slot->io->SRS12 & SFR12_ERROR_INTERRUPT) )
			return SDIO_ERR_TIMEOUT;
		slot->io->SRS12 = SFR12_BUFFER_READ_READY;
		for(i=0; i<slot->blksize; i+=4) {
			unsigned int tmp = slot->io->SRS8;
			//		printf( " 0x%08x\n", tmp);
			writel(tmp, buf);
			buf += 4;
		}
		blkcnt--;
	}
	return 0;
}

static int SDIOHost_SetPower ( struct sdio_slot *slot, unsigned int voltage)
{
	unsigned int tmp;

	// disable SD bus power
	slot->io->SRS10 &= ~(SFR10_BUS_VOLTAGE_MASK | SFR10_SD_BUS_POWER);

	switch (voltage){
		case SFR10_SET_3_3V_BUS_VOLTAGE:
			if (! ( slot->io->SRS16 & SFR16_VOLTAGE_3_3V_SUPPORT ) )
				return SDIO_ERR_WRONG_VALUE;
			// set new volatage value
			tmp |= SFR10_SET_3_3V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
			break;
		case SFR10_SET_3_0V_BUS_VOLTAGE:
			if (! ( slot->io->SRS16 & SFR16_VOLTAGE_3_0V_SUPPORT ) )
				return SDIO_ERR_WRONG_VALUE;
			// set new volatage value
			tmp |= SFR10_SET_3_0V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
			break;
		case SFR10_SET_1_8V_BUS_VOLTAGE:
			if (! (slot->io->SRS16 & SFR16_VOLTAGE_1_8V_SUPPORT ) )
				return SDIO_ERR_WRONG_VALUE;
			// set new volatage value
			tmp |= SFR10_SET_1_8V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
			break;
		case 0:
			// disable bus power
			tmp &= ~SFR10_SD_BUS_POWER;
			break;
		default: return SDIO_ERR_WRONG_VALUE;
	}

	slot->io->SRS10 = tmp;
	return SDIO_ERR_NO_ERROR;
}

static int SDIOHost_SetSDCLK ( struct sdio_slot *slot, unsigned int *frequency /* in KHZ */ )
{
	unsigned int baseclk; /* in KHZ */
	unsigned int i;
	unsigned int tmp,tmp2;
	unsigned int status;

	// set SD clock off
	tmp = slot->io->SRS11;
	slot->io->SRS11 = tmp & ( ~SFR11_SD_CLOCK_ENABLE );

	//read base clock frequency for SD clock in kilo herz
	//baseclk = SFR16_GET_BASE_CLK_FREQ_MHZ( slot->io->SRS16 ) * 1000;

	//if ( !baseclk ){
	//	baseclk = SYTEM_CLK_KHZ;
	//}
	baseclk = CFG_AHB_SDIO_CLOCK;

	for ( i = 1; i < 2046; i++ ){
		if ( ( baseclk / i ) <= *frequency ){
			break;
		}
		if(((baseclk / i) == *frequency) && (baseclk % i) == 0)
			break;
	}

	TRACE(KERN_DEBUG,"SetClk %d=%d/%d\n",*frequency,baseclk,i);

	// read current value of SFR11 register
	tmp = slot->io->SRS11;

	// clear old frequency base settings
	tmp &= ~SFR11_SEL_FREQ_BASE_MASK;

	// Set SDCLK Frequency Select and Internal Clock Enable
	tmp2 =((i / 2) << 8);
	tmp |=  (tmp2 & 0xff00) | ((tmp2 & 0x30000) >> 10);

	tmp |= SFR11_INT_CLOCK_ENABLE;
	slot->io->SRS11 = tmp;

	// wait for clock stable is 1
	status = WaitForValue(&slot->io->SRS11, SFR11_INT_CLOCK_STABLE, 1, COMMANDS_TIMEOUT );
	if ( status )
		return status;

	// set SD clock on
	tmp = slot->io->SRS11;
	slot->io->SRS11 = (tmp | SFR11_SD_CLOCK_ENABLE);

	// write to frequency the real value of set frequecy
	*frequency = baseclk / i;

	return SDIO_ERR_NO_ERROR;

}




static BYTE SDIOHost_CheckDeviceType( struct sdio_slot *slot )
{
	BYTE DeviceType = 0;
	BYTE status;

	// do not check status because MMC card doesn't respond
	// on this command if it is not initialized
	status = SDIOHost_ExecCMD55Command( slot );

	if ( status ){

	} else{
		// execute ACMD41 command
		status = SDIOHost_ExecCardCommand(slot, SDMMC_ACMD41, 0);
	}

	do{
		// if no response on ACMD41 command then check
		// CMD1 command response
		if ( status == SDIO_ERR_COMMAND_TIMEOUT_ERROR ){

			// execute CMD0 command to go to idle state
			status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD0, 0);
			if ( status )
				return status;

			// execute CMD1 command
			status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD1, 0);
			switch ( status ){
				case SDIO_ERR_COMMAND_TIMEOUT_ERROR:
					// it is not MMC card
					break;
				case SDIO_ERR_NO_ERROR:
					DeviceType = SDIOHOST_CARD_TYPE_MMC;
					break;
				default:
					return status;
			}

			if ( DeviceType == SDIOHOST_CARD_TYPE_MMC )
				break;
		} else
			if ( status == SDIO_ERR_NO_ERROR )
				DeviceType = SDIOHOST_CARD_TYPE_SDMEM;
			else
				return status;

		// execute CMD0 command to go to idle state
		status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD0, 0);
		if ( status )
			return status;

		status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD5, 0);
		switch ( status ){
			case SDIO_ERR_COMMAND_TIMEOUT_ERROR:
				break;
			case SDIO_ERR_NO_ERROR:
				// it can be a SDIO device or combo device
				DeviceType |= SDIOHOST_CARD_TYPE_SDIO;
				break;
			default:
				return status;
		}
	}while(0);

	if ( DeviceType == 0 )
	{
		TRACE(KERN_DEBUG,"Err, device type = 0\n");
		return SDIO_ERR_UNUSABLE_CARD;
	}
	slot->DeviceType = DeviceType;
	slot->IsSelected = 0;
	slot->RCA = 0;

	// execute CMD0 command to go to idle state
	status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD0, 0);
	if ( status )
		return status;
	return SDIO_ERR_NO_ERROR;
}



static int SDIOHost_ResetCard( struct sdio_slot *slot )
{
	return SDIOHost_ExecCardCommand(slot, SDMMC_CMD0, 0);
}

static int SDIOHost_SetTimeout ( struct sdio_slot *slot, unsigned int timeout )
{

	if( !(timeout & SFR11_TIMEOUT_MASK) )
		return SDIO_ERR_WRONG_VALUE;

	slot->io->SRS11 &= SFR11_TIMEOUT_MASK;
	slot->io->SRS11 |= timeout;

	return SDIO_ERR_NO_ERROR;
}

static int GetMemCardOCR(struct sdio_slot *slot, unsigned int *OCR)
{
	unsigned int command;
	int status;

	*OCR = 0;
	if ((slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM) ||
			(slot->DeviceType == SDIOHOST_CARD_TYPE_MMC)) {
		if (slot->DeviceType == SDIOHOST_CARD_TYPE_MMC) {
			command = SDMMC_CMD1;
		} else {
			status = SDIOHost_ExecCMD55Command(slot);
			if (status)
				return status;
			command = SDMMC_ACMD41;
		}

		// execute command to check a card OCR register
		status = SDIOHost_ExecCardCommand(slot, command, 0);
		if (status == SDIO_ERR_NO_ERROR)
			*OCR = slot->response[0];
	}
	return SDIO_ERR_NO_ERROR;
}

static int SDIOHOST_MatchVoltage(struct sdio_slot *slot, DWORD OCR, DWORD *CardVoltage, 
		DWORD *ControllerVoltage)
{
	/// host voltage capabilities
	DWORD HostCapabilities;

	*CardVoltage = 0;
	*ControllerVoltage = 0;

	HostCapabilities = slot->io->SRS16;
	// check the voltage capabilities of the SDIO host controller and a card
	// to set appriopriate voltage
	do{
		if ( HostCapabilities & SFR16_VOLTAGE_3_3V_SUPPORT ){
			if ( OCR & SDCARD_REG_OCR_3_3_3_4 ){
				*CardVoltage = SDCARD_REG_OCR_3_3_3_4;
				*ControllerVoltage = SFR10_SET_3_3V_BUS_VOLTAGE;
				break;
			}
			if ( OCR & SDCARD_REG_OCR_3_2_3_3 ){
				*CardVoltage = SDCARD_REG_OCR_3_2_3_3;
				*ControllerVoltage = SFR10_SET_3_3V_BUS_VOLTAGE;
				break;
			}
		}
		if ( HostCapabilities & SFR16_VOLTAGE_3_0V_SUPPORT ){
			if ( OCR & SDCARD_REG_OCR_3_0_3_1 ){
				*CardVoltage = SDCARD_REG_OCR_3_0_3_1;
				*ControllerVoltage = SFR10_SET_3_0V_BUS_VOLTAGE;
				break;
			}
			if ( OCR & SDCARD_REG_OCR_2_9_3_0 ){
				*CardVoltage = SDCARD_REG_OCR_2_9_3_0;
				*ControllerVoltage = SFR10_SET_3_0V_BUS_VOLTAGE;
				break;
			}
		}
	}while(0);
	return SDIO_ERR_NO_ERROR;
}

/*
   static int SetMemCardVoltage(struct sdio_slot *slot, DWORD Voltage, BYTE F8, unsigned int *CCS)
   {
   int status, Condition;
   volatile DWORD Delay;
   unsigned int command, argument;

   argument = 0;
   if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM)
   command = SDMMC_ACMD41;
   else
   command = SDMMC_CMD1; // MMC

   if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM){
// if card responsed on CMD8 command
// set host hight capacity flag to 1
if (F8)
argument |= SDCARD_ACMD41_HCS;
} else{ // MMC card
}

Delay = 10000;
argument |= Voltage;

do{
if (command == SDMMC_ACMD41) {
status = SDIOHost_ExecCMD55Command(slot);
if (status)
return SDIO_ERR_UNUSABLE_CARD;
}

// execute ACMD41 or CMD1 command with setting the supply voltage as argument
SDIOHost_ExecCardCommand(slot, command, argument);
if (status != SDIO_ERR_NO_ERROR)
return SDIO_ERR_UNUSABLE_CARD;
Condition = ((slot->response[0] & SDCARD_REG_OCR_READY) != 0);
// if no response from card it is unusable return error code
}while (!Condition && --Delay); // wait until card will be ready

if (Delay == 0) // card is busy to much time
return SDIO_ERR_UNUSABLE_CARD;

if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM){
// check CCS card flag
if (slot->response[0] & SDCARD_REG_OCR_CCS)
 *CCS = 1;
 else
 *CCS = 0;
 }
 else if (slot->DeviceType == SDIOHOST_CARD_TYPE_MMC){
 if (slot->response[0] & MMC_REG_OCR_SECTOR_MODE)
 *CCS = 1;
 else
 *CCS = 0;
 }
 return SDIO_ERR_NO_ERROR;
 }
 */

static BYTE SetMemCardVoltage(struct sdio_slot *slot, DWORD Voltage, BYTE F8, BYTE *CCS)
{
	BYTE status, Condition;
	volatile DWORD Delay;
	unsigned int command, argument;

	argument = 0;
	if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM){
		command = SDMMC_ACMD41;
	}
	else
		command = SDMMC_CMD1; // MMC

	if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM){
		// if card responsed on CMD8 command
		// set host hight capacity flag to 1
		if (F8)
			argument |= SDCARD_ACMD41_HCS;
	}
	else{ // MMC card
	}

	Delay = 10000;
	argument |= Voltage;

	do{
		if (command == SDMMC_ACMD41){
			status = SDIOHost_ExecCMD55Command(slot);
			if (status)
				return SDIO_ERR_UNUSABLE_CARD;
		}

		// execute ACMD41 or CMD1 command with setting the supply voltage as argument
		status = SDIOHost_ExecCardCommand(slot, command, argument);
		if (status != SDIO_ERR_NO_ERROR)
			return SDIO_ERR_UNUSABLE_CARD;

		Condition = ((slot->response[0] & SDCARD_REG_OCR_READY) != 0);
		// if no response from card it is unusable return error code
	}while (!Condition && --Delay); // wait until card will be ready

	if (Delay == 0) // card is busy to much time
		return SDIO_ERR_UNUSABLE_CARD;

	if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM){
		// check CCS card flag
		if (slot->response[0] & SDCARD_REG_OCR_CCS)
			*CCS = 1;
		else
			*CCS = 0;
	}
	else if (slot->DeviceType == SDIOHOST_CARD_TYPE_MMC){
		if (slot->response[0] & MMC_REG_OCR_SECTOR_MODE)
			*CCS = 1;
		else
			*CCS = 0;
	}
	return SDIO_ERR_NO_ERROR;
}


static int SDIOHost_ReadRCA( struct sdio_slot *slot )
{
	int status;
	unsigned int argument;
	// RCA address for RCA cards for MMC cards
	DWORD RCA = 0x1000;

	RCA--;
	if ( slot->DeviceType == SDIOHOST_CARD_TYPE_MMC ){
		slot->RCA = RCA;
		argument = (DWORD)slot->RCA << 16;
	}else{
		argument = 0;
	}

	status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD3, argument );
	if ( status != SDIO_ERR_NO_ERROR )
		return status;

	if ( slot->DeviceType != SDIOHOST_CARD_TYPE_MMC ){
		//get RCA CMD 3
		slot->RCA = (slot->response[0] >> 16) & 0xFFFF;
	}
	return SDIO_ERR_NO_ERROR;
}

static int SDIOHost_SelectCard( struct sdio_slot *slot, WORD RCA )
{
	unsigned int command;
	int status;

	if ( ( RCA == slot->RCA ) && slot->IsSelected )
		return SDIO_ERR_NO_ERROR;

	if ( ( RCA == 0 ) && !slot->IsSelected )
		return SDIO_ERR_NO_ERROR;

	if ( RCA != 0 ) /* select */
		command = SDMMC_CMD7_SEL;
	else
		command = SDMMC_CMD7_DES;

	status = SDIOHost_ExecCardCommand(slot, command, (DWORD)RCA << 16);
	if ( status == 0 ){
		if ( RCA )
			slot->IsSelected = 1;
		else
			slot->IsSelected = 0;
	}
	return status;
}

static int SDIOHost_AccessCCCR ( struct sdio_slot *slot, BYTE TransferDirection, 
		void *Data, BYTE DataSize, BYTE RegisterAddress)
{
	return SDIO_ERR_NO_ERROR;
}


static int SDIOHost_ReadSCR( struct sdio_slot *slot )
{
	BYTE Buffer[8];
	DWORD SCR;
	int status;

	status = SDIOHost_SelectCard( slot, slot->RCA );
	if ( status )
		return status;  

	status = SDIOHost_ExecCMD55Command( slot ); 
	if ( status )
		return status;    

	// the size of SCR register is 64 bits (8 bytes)
	/*  Block Count: 1
	 *  Block Size:  8
	 */
	slot->blkcnt  = 1;
	slot->blksize = 8;
	status = SDIOHost_ExecCardCommand( slot, SDMMC_ACMD51, 0 ); 
	if ( status )
		return status;
	SDIOHost_ReadDataBlock(slot, Buffer);
	status = SDIOHost_ExecCardCommand( slot, SDMMC_CMD12, 0 ); 

	// get more significant 32 bits from SCR register
	SCR = *((DWORD*)Buffer);
	SCR = ((SCR & 0xFF) << 24) 
		| ((SCR & 0xFF000000) >> 24) 
		| ((SCR & 0xFF0000) >> 8) 
		| ((SCR & 0xFF00) << 8);    

	slot->SupportedBusWidths = ( SCR & SDCARD_REG_SCR_SBW_MASK ) >> 16;
	slot->SpecVersNumb = ( SCR & SDCARD_REG_SCR_SPEC_VER_MASK ) >> 24;

	return SDIO_ERR_NO_ERROR;
}

static int MemoryCard_SetBlockLength(struct sdio_slot *slot, DWORD BlockLength)
{
	int status;

	status = SDIOHost_SelectCard( slot, slot->RCA );
	if(status)
		return status;
	status = SDIOHost_ExecCardCommand( slot, SDMMC_CMD16, BlockLength );
	return status;
}

static int SDIOHost_Configure(struct sdio_slot *slot, int Cmd, void *data, unsigned int *dsize)
{
	int status;

	switch( Cmd ){
		case SDIOHOST_CONFIG_SET_CLK:
			if (*dsize == sizeof(WORD)){
				status = SDIOHost_SetSDCLK(slot, data );
				if ( status ){
					TRACE(KERN_DEBUG, "SDIOHost_Configure", "Set host CLK to %d failed\n", *((WORD*)data) );
					return status;
				}
			}
			else {
				TRACE(KERN_DEBUG, "SDIOHost_Configure", "dsize should be 2 but is %d\n", *dsize);
				return SDIO_ERR_WRONG_VALUE;
			}
			break;
		default:
			return SDIO_ERR_WRONG_VALUE;
	}

	return SDIO_ERR_NO_ERROR;
}

int SDIOHost_DeviceAttach( struct sdio_slot *slot )
{
	int status;
	unsigned int cur_voltage = SFR10_SET_3_3V_BUS_VOLTAGE;
	unsigned int frequency = 400, size;
	unsigned int tmp, CardVoltage, CCS;
	int FlagF8 = 0;
	DWORD CurrentControllerVoltage = SFR10_SET_3_3V_BUS_VOLTAGE;
	DWORD NewControllerVoltage = SFR10_SET_3_3V_BUS_VOLTAGE;
	DWORD OcrIo = 0xFFFFFF, OcrMem = 0xFFFFFF;


	//	 dissable mmc8 mode
	SDIO_REG_HSFR0 &=  ~(1 << (24 + slot->id));

	// set 4 bit bus mode
	slot->io->SRS10  &=  (SFR10_DATA_WIDTH_4BIT);

	// enable bus power
	status = SDIOHost_SetPower( slot, cur_voltage ); 
	if ( status )
		return status;

	// SD clock supply before Issue a SD command
	// clock frequency is set to 400kHz
	status = SDIOHost_SetSDCLK ( slot, &frequency );
	if ( status )
		return status;

	status = SDIOHost_ResetCard( slot );
	if ( status ){
		return status;
	}

	// function checks the type of card device
	// it can be MMC, SD memory or SDIO
	status = SDIOHost_CheckDeviceType( slot );
	if ( status )
		return status;

	// check if card supports High Capacity Memory.
	if ( slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM ){
		// VHS voltage 2.7-3.6V
		status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD8, 1 << 8);

		switch ( status ){
			case SDIO_ERR_COMMAND_TIMEOUT_ERROR:
				FlagF8 = 0;
				break;
			case SDIO_ERR_NO_ERROR:
				FlagF8 = 1;
				break;
			default:
				return status;
		}
	}

	/*
	// get OCR register from IO card
	if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDIO){
	status = GetIoCardOCR(slot, &OcrIo);
	if (status)
	return status;        
	}
	 */

	// get OCR register from memory card
	if ((slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM) || (slot->DeviceType == SDIOHOST_CARD_TYPE_MMC)) {
		status = GetMemCardOCR(slot, &OcrMem);
		if (status)
			return status;
	}

	// match card voltage and controller voltage settings
	status = SDIOHOST_MatchVoltage(slot, (OcrMem & OcrIo), &CardVoltage, 
			&NewControllerVoltage);
	if (status)
		return status;

	if (NewControllerVoltage != CurrentControllerVoltage){
		// change controller voltage
		status = SDIOHost_SetPower(slot, CurrentControllerVoltage); 
		if (status)
			return status;
	}

	status = SDIOHost_ResetCard(slot);    
	if (status)
		return status;

	// check if card supports High Capacity Memory.
	if ( slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM ){
		// VHS voltage 2.7-3.6V
		status = SDIOHost_ExecCardCommand( slot, SDMMC_CMD8, 1 << 8 );
		switch ( status ){
			case SDIO_ERR_COMMAND_TIMEOUT_ERROR:
				FlagF8 = 0;
				break;
			case SDIO_ERR_NO_ERROR:
				FlagF8 = 1;
				break;
			default:
				return status;
		}
	}

	/*
	// set voltage for SDIO card
	if (slot->DeviceType & SDIOHOST_CARD_TYPE_SDIO){
	status = SetIoCardVoltage(slot, CardVoltage);
	if (status)
	return status;
	}
	 */

	//set voltage for memory card
	if ((slot->DeviceType & SDIOHOST_CARD_TYPE_SDMEM) || (slot->DeviceType == SDIOHOST_CARD_TYPE_MMC)) {
		status = SetMemCardVoltage(slot, CardVoltage, FlagF8, &CCS);
		if (status)
			return status;
		// set device capacity info relay on CCS flag from card OCR register 
		if (CCS)
			slot->DeviceCapacity = SDIOHOST_CAPACITY_HIGH;
		else
			slot->DeviceCapacity = SDIOHOST_CAPACITY_NORMAL;
	}

	if ( slot->DeviceType != SDIOHOST_CARD_TYPE_SDIO ) {
		status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD2, 0);
		if ( status )
			return status;
	}

	status = SDIOHost_ReadRCA(slot);
	if (status)
		return status;

	slot->inserted = 1;

	// for SD memory only card get information about card from SCR register
	if ( slot->DeviceType == SDIOHOST_CARD_TYPE_SDMEM ){
		status = SDIOHost_ReadSCR( slot );
		if ( status )
			return status;
	}

	// for SDIO or combo cards read CCCR to get information about supported bus widths
	if ( slot->DeviceType & SDIOHOST_CARD_TYPE_SDIO ){
		BYTE CardCapability = 0;

		// read bus card capabilities
		status = SDIOHost_AccessCCCR( slot, SDIOHOST_CCCR_READ, &CardCapability,
				sizeof(CardCapability), SDCARD_CCCR_CARD_CAPABILITY );
		if (status)
			return status;

		if ( ( CardCapability & SDCARD_CCR_LSB ) != 0
				&& ( ( CardCapability & SDCARD_CCR_4BLS ) ) == 0 ){
			slot->SupportedBusWidths = SDIOHOST_BUS_WIDTH_1;
		}
		else
			slot->SupportedBusWidths = SDIOHOST_BUS_WIDTH_1 | SDIOHOST_BUS_WIDTH_4;
	}

	if( slot->DeviceType & SDIOHOST_CARD_TYPE_MMC ){

	}

	status = MemoryCard_SetBlockLength(slot, SDMMC_SECTOR_SIZE );
	if(status)
		return status;

	//	frequency = 25000;
	frequency = CFG_SDIO_CLOCK;
	size = 2;
	status = SDIOHost_Configure( slot, SDIOHOST_CONFIG_SET_CLK, &frequency, &size );
	if(status) {
		TRACE(KERN_ERROR, "Cannot change frequency to 25M\n");
		return status;
	}
	TRACE(KERN_DEBUG, "SD Clock is set to %dKHz\n", frequency);

	return 0;
}

static int SDIOHost_HostInitialize( )
{
	volatile unsigned int tmp;
	int status;

	//HostBusMode = HOST_BUS_MODE;

	// reset controller for sure
	SDIO_REG_HSFR0 |= HSFR0_SOFTWARE_RESET;
	//	status = WaitForValue( &SDIO_REG_HSFR0, HSFR0_SOFTWARE_RESET, 0, COMMANDS_TIMEOUT );

#if DEBOUNCING_TIME > 0xFFFFFF
#   error WRONG VALUE OF DEBOUNCING TIME IN CONFIG.H FILE
#endif
	SDIO_REG_HSFR1 = DEBOUNCING_TIME;
	return SDIO_ERR_NO_ERROR;
}

static int SDIOHost_CheckSlots(struct sdio_slot *slots, struct sdio_slot **active_slot)
{
	int i;
	int status;

	for ( i = 0; i < 4; i ++ ) {
		slots[i].io = (void *)(SDIO_REGISTERS_OFFSET + (i + 1) * 0x100);    
		if( (SDIO_REG_HSFR0 & (0x10000 << i)) == 0 )
			continue;
		status = WaitForValue( &slots[i].io->SRS9, SFR9_CARD_STATE_STABLE, 1, COMMANDS_TIMEOUT );
		if (status) {
			return status;
		}
		slots[i].io->SRS13 = 0xffffffff;
		slots[i].id = i;
		SDIOHost_SetTimeout(&slots[i], SFR11_TIMEOUT_TMCLK_X_2_POWER_24 );
		status = SDIOHost_DeviceAttach( &slots[i] );
		if (status) {
			return status;
		} else { /* FIXME */
			*active_slot = &slots[i];
			break;
		}
	}
	return SDIO_ERR_NO_ERROR;
}


static int MemoryCard_DataTransfer( struct sdio_slot *slot, unsigned int address, void *buff, unsigned int count)
{
	unsigned int cmd, argument = 0;
	int status;
	
	
	status = SDIOHost_SelectCard( slot, slot->RCA );
	if(status)
		return status;

	if ( slot->DeviceCapacity == SDIOHOST_CAPACITY_NORMAL ){
		// Data address is in byte units in a Standard Capacity SD memory card and MMC memory card
		argument = address * SDMMC_SECTOR_SIZE;
	} else if (slot->DeviceCapacity == SDIOHOST_CAPACITY_HIGH){
		argument = address;
	}

	slot->blkcnt  = count;
	slot->blksize = SDMMC_SECTOR_SIZE;

	if(count == 1)
		cmd = SDMMC_CMD17;
	else
		cmd = SDMMC_CMD18;
	status = SDIOHost_ExecCardCommand(slot, cmd, argument);
	if(status)
		return status;
	SDIOHost_ReadDataBlock(slot, buff);
	status = SDIOHost_ExecCardCommand(slot, SDMMC_CMD12, 0);
	return SDIO_ERR_NO_ERROR;
}

/*=============================================================*/
/*  External Interfaces                                        */
/*=============================================================*/

struct sdio_slot slots[2], *active_slot;

int sdmmc_boot(int boot_mode)
{
	int status = 0;
	int i = 0;
	unsigned char * buf = (unsigned char *)DEV_RX_BUF;
	int sec_cnt = 0;
	int start_sec = 0;

	memset(slots, 0, sizeof(slots));

	status = SDIOHost_HostInitialize();  
	if(status != 0)
	{
		TRACE(KERN_ERROR, "Init SDMMC failed\n");
		return status;
	}

	for ( i = 0; i < 2; i ++ ) 
	{
		TRACE(KERN_INFO, "[%d]\n", i);	

		slots[i].io = (void *)(SDIO_REGISTERS_OFFSET + (i + 1) * 0x100);    
		while ((SDIO_REG_HSFR0 & (0x10000 << i)) == 0)
		{
			TRACE(KERN_ERROR," The slot[%d] is busy\n",(i+1));
		}

		slots[i].io->SRS13 = 0xffffffff;
		slots[i].id = i;
		SDIOHost_SetTimeout(&slots[i], SFR11_TIMEOUT_TMCLK_X_2_POWER_24 );

		status = SDIOHost_DeviceAttach( &slots[i] );
		if (status) 
		{
			TRACE(KERN_DEBUG, "Attach failed\n");
			continue;
		}
		else	
		{ /* Find Card */
			status = MemoryCard_DataTransfer( &slots[i], CFG_UBOOT_ADDR, buf, 1);
			if(status)
			{
				TRACE(KERN_ERROR, "Failed to read sector %u\n", CFG_UBOOT_ADDR);	
				continue; 
			}

			memcpy(&head, buf, XL_HEAD_SIZE); 
			status = check_header(&head);
			if(status)
				continue; 		

			sec_cnt = (head.size + SDMMC_SECTOR_SIZE - 1)/SDMMC_SECTOR_SIZE;
			start_sec = CFG_UBOOT_ADDR + (head.offset + SDMMC_SECTOR_SIZE - 1)/SDMMC_SECTOR_SIZE;
			status = MemoryCard_DataTransfer( &slots[i], start_sec, head.entry, sec_cnt);
			if(status)
			{
				TRACE(KERN_ERROR, "Failed to read  u-boot\n");
				continue;
			}

			if(checksum32(head.entry, head.size) != head.loader_cksum)
			{
				TRACE(KERN_ERROR, "chcksum fail\n");
				continue;
			}

			enter_entry(head.entry, boot_mode);
		}
	}



	return status;
}

int sdmmc_continue_boot(int boot_mode)
{
	int status = 0;
	int i = 0;
	unsigned char * buf = (unsigned char *)DEV_RX_BUF;
	int sec_cnt = 0;
	int start_sec = 0;
	int freq = 25000;

	memcpy(slots, 0x0704E020, sizeof(slots));

	if(slots[1].inserted)
		i= 1;
	

	status = SDIOHost_SetSDCLK(&slots[i], &freq);
	if(status)
		return status;


	TRACE(KERN_INFO, "go on at [%d]\n", i);	

			status = MemoryCard_DataTransfer( &slots[i], CFG_UBOOT_ADDR, buf, 1);
			if(status)
			{
				TRACE(KERN_ERROR, "Failed to read sector %u\n", CFG_UBOOT_ADDR);	
				return; 
			}

			memcpy(&head, buf, XL_HEAD_SIZE); 
			status = check_header(&head);
			if(status)
				return; 		
		
			sec_cnt = (head.size + SDMMC_SECTOR_SIZE - 1)/SDMMC_SECTOR_SIZE;
			start_sec = CFG_UBOOT_ADDR + (head.offset + SDMMC_SECTOR_SIZE - 1)/SDMMC_SECTOR_SIZE;
			status = MemoryCard_DataTransfer( &slots[i], start_sec, head.entry, sec_cnt);
                        if(status)
                        {
                                TRACE(KERN_ERROR, "Failed to read  u-boot\n");
                                return;
                        }
			
			if(checksum32(head.entry, head.size) != head.loader_cksum)
			{
				TRACE(KERN_ERROR, "chcksum fail\n");
                                return;
			}

            		enter_entry(head.entry, boot_mode);

	return status;
}
