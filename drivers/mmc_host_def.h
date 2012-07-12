/*
 * FILE : mmc_host_def.h
 * Desc : define of mmc controler
 * Author : jinying.wu
 * Data : 
 * Modify :
 * Version :
 * Group : NuSmart
 */

#ifndef MMC_HOST_DEF_H
#define MMC_HOST_DEF_H

/* Evatronix SDIO-HOST Register Map */
#define HRS0    (0x000)

#define SRS_OFFSET  0x100
/* Slot Register Set */

#define RESPONSE_136BIT(cmd)  ((((cmd) >> 16) & 3) == 1)
#define RESPONSE_48BIT(cmd)   (((cmd) >> 16) & 2)

struct slot_register_set {
	unsigned int  SRS0;
	unsigned int  SRS1;
	unsigned int  SRS2;
	unsigned int  SRS3;
	unsigned int  SRS4;
	unsigned int  SRS5;
	unsigned int  SRS6;
	unsigned int  SRS7;
	unsigned int  SRS8;
	unsigned int  SRS9;
	unsigned int  SRS10;
	unsigned int  SRS11;
	unsigned int  SRS12;
	unsigned int  SRS13;
	unsigned int  SRS14;
	unsigned int  SRS15;
	unsigned int  SRS16;
	unsigned int  SRS17;
	unsigned int  SRS18;
	unsigned int  SRS19;
	unsigned int  SRS20;
	unsigned int  SRS21;
	unsigned int  SRS22;
	unsigned int  SRS23;
	unsigned int  SRS24;
	unsigned int  SRS25;
	unsigned int  SRS26;
	unsigned int  SRS27;
	unsigned int  SRS28;
	unsigned int  SRS29;
	unsigned int  SRS30;
	unsigned int  SRS31;
	unsigned int  SRS32;
	unsigned int  SRS33;
	unsigned int  SRS34;
	unsigned int  SRS35;
	unsigned int  SRS36;
	unsigned int  SRS37;
	unsigned int  SRS38;
	unsigned int  SRS39;
	unsigned int  SRS40;
	unsigned int  SRS41;
	unsigned int  SRS42;
	unsigned int  SRS43;
	unsigned int  SRS44;
	unsigned int  SRS45;
	unsigned int  SRS46;
	unsigned int  SRS47;
	unsigned int  SRS48;
	unsigned int  SRS49;
	unsigned int  SRS50;
	unsigned int  SRS51;
	unsigned int  SRS52;
	unsigned int  SRS53;
	unsigned int  SRS54;
	unsigned int  SRS55;
	unsigned int  SRS56;
};

#endif /* MMC_HOST_DEF_H */
