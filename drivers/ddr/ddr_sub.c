#include "dw_ddr.h"
#include <common.h>
#define TEST_VERSION

	// work_mode = 0xTSPC_WFLB;
	// T = dram type; 
	//     0: ddr3;
	//     1: lpddr2;
	//     2: ddr3l;
	// SP = supplier | device part number;
	//     0: micron;
	//         0: mt41j512m8-125
	//         1: mt41j256m16-125
	//     1: samsung;
	//         0: k4b2g1646c-h9
	//         1: k4b2g0846b-f8
	//         2: k4b2g0846d-k0
	//     2: hynix;
	//         0: h9tcnnn8ldmmr-2gbit-800
	//         1: h5tq2g83bfr-h9
	//         2: h5tq2g83bfr-pb
  //         3: h5tq2g83cfr-h9
	//     3: elpeda;
	//         0: edj2116debg-dj
	//     4: jedec;
	//         0: jedec_ddr3_2gb_x8_800e
	//         1: jedec_lpddr2s4_4gb_x32_1066
	// C = capacity
	//     0: 1Gb x 1 rank, cs0 enable;
	//     1: 2Gb x 1 rank, cs0 enable;
	//     2: 4Gb x 1 rank, cs0 enable;
	//     3: 8Gb x 1 rank, cs0 enable;
	//     4: 1Gb x 1 rank, cs1 enable;
	//     5: 2Gb x 1 rank, cs1 enable;
	//     6: 4Gb x 1 rank, cs1 enable;
	//     7: 8Gb x 1 rank, cs1 enable;
	//     8: 1Gb x 2 rank;
	//     9: 2Gb x 2 rank;
	//     a: 4Gb x 2 rank;
	//     b: 8Gb x 2 rank;
	//
	// W = bit width;
	//     0: 8-bit width;
	//     1: 16-bit width;
	//     2: 32-bit width;
	// F = frequency;
	//     0: 400Mhz;
	//     1: 333Mhz;
	//     2: 200Mhz;
	//     3: 100Mhz;
	// L = cas latency;
	// B = burst length;
	//     0: burst 8;
	//     1: burst 4;

unsigned int pu_code;
unsigned int pd_code;
unsigned int addr_fail_num=0;
unsigned int data_fail_num=0;
unsigned int addr_bist_status=0;
unsigned int data_bist_status=0;
unsigned int fail_flag = 0;
unsigned int gtlvl_fail_flag = 0;
unsigned int ddrinit_fail_flag =0;
unsigned int init_fail_time = 0;

void ddr_init();

void delay(unsigned int loops)
{
	while(loops --);
}

void ddr_reset()
{
  REG_WRITE(0x0582108c, REG_READ(0x0582108c) & 0xffffefff | 0x00000000);
  delay(10);
  REG_WRITE(0x0582108c, REG_READ(0x0582108c) & 0xffffefff | 0x00001000);
  delay(100);
}

void arbiter_init(unsigned int work_mode)
{
	unsigned int memmax_dram_type;
	unsigned int memmax_chip_bits;
	unsigned int memmax_cb_loc;
		
	memmax_chip_bits = (work_mode >> 19) & 0x1;
	
	memmax_cb_loc = 32 - 27 - ((work_mode>>16) & 0x3) - memmax_chip_bits - 2 + (work_mode>>12 & 0x3);
	
	if((work_mode >> 28)==1) 
  {
  	memmax_dram_type = 0x2;
  }
  else
  {
  	memmax_dram_type = 0x3;
  }
  
	REG_WRITE(MEMMAX_BASE,(memmax_chip_bits<<28) | (memmax_cb_loc<<24) | (0xc<<16) | (0x2<<8) | (memmax_dram_type<<4) | 0x1);
	REG_WRITE(MEMMAX_BASE+0x00000020,0x00000002);
	REG_WRITE(MEMMAX_BASE+0x00000024,0x00400005);
	REG_WRITE(MEMMAX_BASE+0x00000030,0x00000002);
	REG_WRITE(MEMMAX_BASE+0x00000034,0x04000002);
	REG_WRITE(MEMMAX_BASE+0x00000060,0x04000001);
	REG_WRITE(MEMMAX_BASE+0x00000064,0x01000004);
	REG_WRITE(0x07200500,0x30);
}
 
void lpddr_drv()
{
	debug("\n\n");
	debug("LPDDR_DRV: temprature check value cs0=0x%d, cs1=0x%d\n", 
	        REG_READ(DDR_BASE_ADDR + 0x74) >> 16 & 0x0000000f,
	        REG_READ(DDR_BASE_ADDR + 0x74) >> 20 & 0x0000000f);
	debug("\n\n");
}

void enable_dynamic_cke()
{
	// enable auto entry and exit
	REG_WRITE(DDR_BASE_ADDR + 0x58, (REG_READ(DDR_BASE_ADDR + 0x58) & 0x0000ffff | 0x01010000) ); 
	// the auto entry counter should be refined according to application    
	REG_WRITE(DDR_BASE_ADDR + 0x5c, (REG_READ(DDR_BASE_ADDR + 0x5c) & 0xffff0000 | 0x0000000a) ); 
	REG_WRITE(DDR_BASE_ADDR + 0x60, (REG_READ(DDR_BASE_ADDR + 0x60) & 0xffff0000 | 0x0000000a) ); 
	REG_WRITE(DDR_BASE_ADDR + 0x64, (REG_READ(DDR_BASE_ADDR + 0x64) & 0xffff0000 | 0x000000aa) ); 
	REG_WRITE(DDR_BASE_ADDR + 0x68, (REG_READ(DDR_BASE_ADDR + 0x68) & 0xffff0000 | 0x00000000) );
}

void enable_dynamic_odt(unsigned int work_mode)
{ //enable SoC dynamic ODT
  if ((work_mode >> 28) == 1)
  {// lpddr2
  	REG_WRITE(DDR_BASE_ADDR + 0x348, 0x00000000);  //define tsel
  }
  else
  {
  	REG_WRITE(DDR_BASE_ADDR + 0x348, 0x00030300);  //define tsel
  }
}

void zq_cal(unsigned int work_mode)
{
	unsigned int type=3;
	unsigned int i;
	unsigned int sum=0;
	unsigned int pu_inc[6], pd_inc[6], pu_dec[6], pd_dec[6];

	if ((work_mode >> 28) == 0) {type = 0x3;}           //ddr3
	else if ((work_mode >> 28) == 1) {type = 0x0;}      //lpddr2
	else if ((work_mode >> 28) == 2) {type = 0x1;}      //ddr3l

	REG_WRITE(DDR_BASE_ADDR + 0x37c, 0x00002064 | (type << 3) );
	
	for (i=0; i<6; i++) 
	{
		REG_WRITE(DDR_BASE_ADDR + 0x37c, 0x00002066 | (type << 3));    //cal clear
		REG_WRITE(DDR_BASE_ADDR + 0x37c, 0x00002064 | (type << 3));
		REG_WRITE(DDR_BASE_ADDR + 0x37c, 0x00002065 | (type << 3));    //cal go
		while (!(REG_READ(DDR_BASE_ADDR + 0x384) & 0x80000000));       //waiting for cal done  
		pu_inc[i] = REG_READ(DDR_BASE_ADDR + 0x384) & 0x000000ff;
		pu_dec[i] = (REG_READ(DDR_BASE_ADDR + 0x384) & 0x0000ff00) >> 8;
		sum = sum + pu_inc[i] + pu_dec[i];
		
		debug ("pu_inc is %d, pu_dec is %d\n", pu_inc[i], pu_dec[i]);
	}
	pu_code = sum/12;
	sum=0;
	for (i=0; i<6; i++) 
	{
		REG_WRITE(DDR_BASE_ADDR + 0x37c, (pu_code << 16) | 0x00002066 | (type << 3));    //cal clear
		REG_WRITE(DDR_BASE_ADDR + 0x37c, (pu_code << 16) | 0x00002064 | (type << 3));
		REG_WRITE(DDR_BASE_ADDR + 0x37c, (pu_code << 16) | 0x00002065 | (type << 3));    //cal go
		while (!(REG_READ(DDR_BASE_ADDR + 0x384) & 0x80000000));       //waiting for cal done  
		pd_inc[i] = (REG_READ(DDR_BASE_ADDR + 0x384) & 0x00ff0000) >> 16;
		pd_dec[i] = (REG_READ(DDR_BASE_ADDR + 0x384) & 0x7f000000) >> 24;
		sum = sum + pd_inc[i] + pd_dec[i];
		
		debug ("pd_inc is %d, pd_dec is %d\n", pd_inc[i], pd_dec[i]);
	}
	pd_code = sum/12;
	debug ("pu_code=%d, pd_code=%d\n", pu_code, pd_code);
	REG_WRITE(DDR_BASE_ADDR + 0x37c, 0x00000000 | (type << 3) );
}

void io_init(unsigned int work_mode)
{
	unsigned int drv_code_H_0, drv_code_L_0, drv_code_H_1, drv_code_L_1, drv_code_H_1_ac, drv_code_L_1_ac, drv_code_H_2, drv_code_L_2;
	unsigned int trm_code_H_DQ, trm_code_L_DQ, trm_code_H_DQS, trm_code_L_DQS;
	unsigned int drv_reg_0, drv_reg_1, drv_reg_2;
	unsigned int trm_reg0, trm_reg1, trm_reg2;
	unsigned int io_mode;

  enable_dynamic_odt(work_mode);
    
	if ((work_mode >> 28) == 0x1)
	{//lpddr2
		drv_code_H_0 = pu_code *6/11;                 //two set of 110ohm driver
		drv_code_L_0 = pd_code *6/11;                 //two set of 110ohm driver
		drv_code_H_1 = pu_code *6/9;                  //two set of 90ohm driver
		drv_code_L_1 = pd_code *6/9;                  //two set of 90ohm driver
		drv_code_H_1_ac = pu_code *6/12;              //two set of 120ohm driver
		drv_code_L_1_ac = pd_code *6/12;              //two set of 120ohm driver
		drv_code_H_2 = pu_code *6/11;                 //two set of 110ohm driver
		drv_code_L_2 = pd_code *6/11;                 //two set of 110ohm driver
		
		drv_reg_0 = 0x30003000 | (drv_code_H_0 << 6) | drv_code_L_0 | (drv_code_H_0 << 22) | (drv_code_L_0 << 16);
		drv_reg_1 = 0x30003000 | (drv_code_H_1_ac << 6) | drv_code_L_1_ac | (drv_code_H_1 << 22) | (drv_code_L_1 << 16);
		drv_reg_2 = 0x30003000 | (drv_code_H_2 << 6) | drv_code_L_2 | (drv_code_H_2 << 22) | (drv_code_L_2 << 16);
	}
	else
	{
		drv_code_H_0 = pu_code *6/11;                 //two set of 110ohm driver
		drv_code_L_0 = pd_code *6/11;                 //two set of 110ohm driver
		drv_code_H_1 = pu_code *6/10;                 //two set of 100ohm driver
		drv_code_L_1 = pd_code *6/10;                 //two set of 100ohm driver
		drv_code_H_1_ac = pu_code *6/8;               //two set of 80ohm driver
		drv_code_L_1_ac = pd_code *6/8;               //two set of 80ohm driver
		drv_code_H_2 = pu_code *6/11;                 //two set of 110ohm driver
		drv_code_L_2 = pd_code *6/11;                 //two set of 110ohm driver
		
		if ((work_mode >> 28) == 0)
		{//ddr3
			io_mode = 0xf;
		}
		else 
		{//ddr3l
			io_mode = 0x7;
		}

		drv_reg_0 = (io_mode << 28) | (io_mode << 12) | (drv_code_H_0 << 6) | drv_code_L_0 | (drv_code_H_0 << 22) | (drv_code_L_0 << 16);
		drv_reg_1 = (io_mode << 28) | (io_mode << 12) | (drv_code_H_1_ac << 6) | drv_code_L_1_ac | (drv_code_H_1 << 22) | (drv_code_L_1 << 16);
		drv_reg_2 = (io_mode << 28) | (io_mode << 12) | (drv_code_H_2 << 6) | drv_code_L_2 | (drv_code_H_2 << 22) | (drv_code_L_2 << 16);
	}
	REG_WRITE(DDR_BASE_ADDR + 0x34c, drv_reg_0);
	REG_WRITE(DDR_BASE_ADDR + 0x350, drv_reg_1);
	REG_WRITE(DDR_BASE_ADDR + 0x354, drv_reg_2);
	REG_WRITE(DDR_BASE_ADDR + 0x364, drv_reg_0);
	REG_WRITE(DDR_BASE_ADDR + 0x368, drv_reg_1);
	REG_WRITE(DDR_BASE_ADDR + 0x36c, drv_reg_2);
	
	debug("drv_code_H_0=%d, drv_code_L_0=%d\n", drv_code_H_0, drv_code_L_0);
	debug("drv_code_H_1=%d, drv_code_L_1=%d\n", drv_code_H_1, drv_code_L_1);
	debug("drv_code_H_1_ac=%d, drv_code_L_1_ac=%d\n", drv_code_H_1_ac, drv_code_L_1_ac);
	debug("drv_code_H_2=%d, drv_code_L_2=%d\n", drv_code_H_2, drv_code_L_2);
	
	
	if ((work_mode >> 28) == 0x1) 
	{ //lpddr2
		trm_reg0 = 0x40004000;
		trm_reg1 = 0x00000000;
		trm_reg2 = 0x40004000;
	}
	else 
	{
		trm_code_H_DQS = pu_code ;                     //60ohm pull-up
		trm_code_L_DQS = pd_code ;                     //60ohm pull-down
		trm_code_H_DQ = pu_code *6/11;                 //110ohm pull-up
		trm_code_L_DQ = pd_code *6/11;                 //110ohm pull-down

		debug("trm_code_H_DQ=%d, trm_code_L_DQ=%d\n", trm_code_H_DQ, trm_code_L_DQ);
		debug("trm_code_H_DQS=%d, trm_code_L_DQS=%d\n", trm_code_H_DQS, trm_code_L_DQS);

		trm_reg0 = 0x70007000 | (trm_code_H_DQ << 6) | trm_code_L_DQ | (trm_code_H_DQS << 22) | (trm_code_L_DQS << 16);
		trm_reg1 = 0x00000000;
		//trm_reg2 = 0x70007000 | (trm_code_H << 6) | trm_code_L | (trm_code_H << 22) | (trm_code_L << 16);
		trm_reg2 = 0x40004000;
	}
	REG_WRITE(DDR_BASE_ADDR + 0x358, trm_reg0);
	REG_WRITE(DDR_BASE_ADDR + 0x35c, trm_reg1);
	REG_WRITE(DDR_BASE_ADDR + 0x360, trm_reg2);
	REG_WRITE(DDR_BASE_ADDR + 0x370, trm_reg0);
	REG_WRITE(DDR_BASE_ADDR + 0x374, trm_reg1);
	REG_WRITE(DDR_BASE_ADDR + 0x378, trm_reg2);
	
}

void mrs(
unsigned int ram_type,
unsigned int mr,
unsigned int mr_value)
{
	//unsigned int mr1[9]={0x60, 0x24, 0x64,
	//	                   0x40, 0x04, 0x44,
	//	                   0x42, 0x03, 0x46};
	//unsigned int i;

	if (ram_type == 1)
	{//lpddr2
		if (mr==3)
		{//lpddr2 dram drv and odt set
			REG_WRITE(DDR_BASE_ADDR + 0x80, REG_READ(DDR_BASE_ADDR + 0x80) & 0xffff0000 |  mr_value  );
			REG_WRITE(DDR_BASE_ADDR + 0x8c, REG_READ(DDR_BASE_ADDR + 0x8c) & 0x0000ffff |  (mr_value << 16) );
		}
	}
	else
	{
		if (mr==1)
		{//ddr3 dram drv and odt set
			REG_WRITE(DDR_BASE_ADDR + 0x78, REG_READ(DDR_BASE_ADDR + 0x78) & 0x0000ffff | (mr_value << 16) );
			REG_WRITE(DDR_BASE_ADDR + 0x88, REG_READ(DDR_BASE_ADDR + 0x88) & 0xffff0000 |  mr_value  );
		}
	}
	
}

void ddr3_workaround(unsigned int work_mode)
{
	if((work_mode >> 28) != 1)
	{
		REG_WRITE(DDR_BASE_ADDR + 0xb8, REG_READ(DDR_BASE_ADDR + 0xb8) & 0xff00ffff);                //placement_en=0
		REG_WRITE(DDR_BASE_ADDR + 0xc0, REG_READ(DDR_BASE_ADDR + 0xc0) & 0xffff00ff | 0x00000700);   //num_q=7
		REG_WRITE(DDR_BASE_ADDR + 0xc8, REG_READ(DDR_BASE_ADDR + 0xc8) & 0xff00ffff | 0x00010000);   //in_order_accept=1
		
		debug("\n\nDDR3 workaround applied!\n\n");
	}
}	

void global_reg_redefine(unsigned int work_mode)
{
	//only issue non-narrow transfer for all ID, memmax ensure this operation
	REG_WRITE(DDR_BASE_ADDR + 0x174, REG_READ(DDR_BASE_ADDR + 0x174) & 0xffff0000 | 0x0);
	//enable sychronous access between axi and databahn interface
	REG_WRITE(DDR_BASE_ADDR + 0x178, REG_READ(DDR_BASE_ADDR + 0x178) & 0xfffffffc | 0x3);
	//workaround for the Self Refresh Entry during Power On
	REG_WRITE(DDR_BASE_ADDR + 0x50, REG_READ(DDR_BASE_ADDR + 0x50) & 0xfff0ffff | 0x0);
	
	if((work_mode >> 28)==1)
	{//lpddr2
		//default setting is 0x20202020, this value got by address training
		REG_WRITE(DDR_BASE_ADDR + 0x310, 0x26262626);
	}
	else
	{
		//default setting is 0x00d90060, change setting
		//[21:19]: rd_dly_sel 3->4
		//[18:16]: phase_detect_sel 1->3
		REG_WRITE(DDR_BASE_ADDR + 0x208, 0x00a30060);
		REG_WRITE(DDR_BASE_ADDR + 0x248, 0x00a30060);
		REG_WRITE(DDR_BASE_ADDR + 0x288, 0x00a30060);
		REG_WRITE(DDR_BASE_ADDR + 0x2c8, 0x00a30060);
		REG_WRITE(DDR_BASE_ADDR + 0x308, 0x00a30060);
	
		//default setting is 0x01121e25, change setting
		//[24]: half_clock_mode 1->0, suitable for freq larger than 157Mhz
		//[22:20]: phase_detect_sel 1->3
		//[15:8]: dll_increment 1e->0a
		//[7:0]: dll_start_point 25->0a
		REG_WRITE(DDR_BASE_ADDR + 0x20c, 0x00320a0a);
		REG_WRITE(DDR_BASE_ADDR + 0x24c, 0x00320a0a);
		REG_WRITE(DDR_BASE_ADDR + 0x28c, 0x00320a0a);
		REG_WRITE(DDR_BASE_ADDR + 0x2cc, 0x00320a0a);
		REG_WRITE(DDR_BASE_ADDR + 0x30c, 0x00320a0a);
		
		//default setting is 0x20202020, this value got by address training
		REG_WRITE(DDR_BASE_ADDR + 0x310, 0x26262626);

		//default setting is 0x00005004, change setting
		//[8]: en_sw_half_cycle 0->1
		REG_WRITE(DDR_BASE_ADDR + 0x344, 0x00005104);
	}
}

void ctrl_bist(
int BIST_Type,
unsigned int start_addr,
unsigned int space,
unsigned int round,
unsigned int print_switch)
{
	unsigned int r=0;
	//unsigned int fail_flag=0;
	unsigned int fail_data_h, fail_data_l, exp_data_h, exp_data_l, fail_addr_h, fail_addr_l;
	//unsigned int conf_data_h, conf_data_l;
	
	if (print_switch) debug("INT_CHK: 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
	REG_WRITE(DDR_BASE_ADDR + 0xd4, REG_READ(DDR_BASE_ADDR + 0xd0));
	if (print_switch) debug("INT_CHK: clear all int, 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
	
	for (r=0; r<round; r++)
	{
	  
	  addr_bist_status = 0;
	  data_bist_status = 0;
	  
		fail_flag=0;
		
		if(BIST_Type == 0)
		{
			REG_WRITE(DDR_BASE_ADDR + 0x94, (REG_READ(DDR_BASE_ADDR + 0x94) & 0xfff0ffff | 0x00000000));  //data bist mode off
			REG_WRITE(DDR_BASE_ADDR + 0x94, (REG_READ(DDR_BASE_ADDR + 0x94) & 0xf0ffffff | 0x01000000));  //address bist mode on
			REG_WRITE(DDR_BASE_ADDR + 0x98, 0x00000000);
			REG_WRITE(DDR_BASE_ADDR + 0x9c, 0x00000000);                                                  //start address, must be0
			REG_WRITE(DDR_BASE_ADDR + 0x94, (REG_READ(DDR_BASE_ADDR + 0x94) & 0xffff00ff | space << 8));  //space is 2**bit[15:8]
			REG_WRITE(DDR_BASE_ADDR + 0x90, (REG_READ(DDR_BASE_ADDR + 0x90) & 0xf0ffffff | 0x01000000));  //bist go!
			
			while (!(REG_READ(DDR_BASE_ADDR + 0xd0) & 0x00000080));                                      //wait bist done
			
			if (REG_READ(DDR_BASE_ADDR + 0x94) & 0x00000002)
			{
				if (print_switch) debug("Controller address PASS for 2^%d space!\n", space);
				addr_bist_status = 1;
			}
			else
			{
				if (print_switch) debug("Controller address FAIL for 2^%d space!\n", space);
				addr_bist_status = 0;
				fail_addr_l = REG_READ(DDR_BASE_ADDR + 0xf4);
				if (print_switch) debug("Controller address FAILed at 0x%x\n", fail_addr_l);
				addr_fail_num ++;
				if (print_switch) debug("Controller address FAIL for %d times!\n", addr_fail_num);
			}
			
			REG_WRITE(DDR_BASE_ADDR + 0xd4, (REG_READ(DDR_BASE_ADDR + 0xd4) & 0xffffff7f | 0x00000080));  //clear the irq
			REG_WRITE(DDR_BASE_ADDR + 0x90, (REG_READ(DDR_BASE_ADDR + 0x90) & 0xf0ffffff | 0x00000000));  //bist stop
		}
		
		else if(BIST_Type == 1)
		{
			REG_WRITE(DDR_BASE_ADDR + 0x94, (REG_READ(DDR_BASE_ADDR + 0x94) & 0xfff0ffff | 0x00010000));  //data bist mode on
			REG_WRITE(DDR_BASE_ADDR + 0x94, (REG_READ(DDR_BASE_ADDR + 0x94) & 0xf0ffffff | 0x00000000));  //address bist mode off
			REG_WRITE(DDR_BASE_ADDR + 0x98, 0x00000000);
			REG_WRITE(DDR_BASE_ADDR + 0x9c, start_addr);                                                  //start address, could be non-0
			REG_WRITE(DDR_BASE_ADDR + 0x94, (REG_READ(DDR_BASE_ADDR + 0x94) & 0xffff00ff | space << 8));  //space is 2**bit[15:8]
			REG_WRITE(DDR_BASE_ADDR + 0x90, (REG_READ(DDR_BASE_ADDR + 0x90) & 0xf0ffffff | 0x01000000));  //bist go!
			
			while (!(REG_READ(DDR_BASE_ADDR + 0xd0) & 0x00000080));                                      //wait bist done
			
			if (REG_READ(DDR_BASE_ADDR + 0x94) & 0x00000001)
			{
				data_bist_status = 1;
				if (print_switch) debug("Controller data PASS at round %d for 2^%d space!\n", r, space);
			}
			else
			{
				if (print_switch) debug("Controller data FAIL for 2^%d space!\n", space);
				exp_data_h = REG_READ(DDR_BASE_ADDR + 0xe8);
				exp_data_l = REG_READ(DDR_BASE_ADDR + 0xe4);
				fail_data_h = REG_READ(DDR_BASE_ADDR + 0xf0);
				fail_data_l = REG_READ(DDR_BASE_ADDR + 0xec);
				fail_addr_h = REG_READ(DDR_BASE_ADDR + 0xf8);
				fail_addr_l = REG_READ(DDR_BASE_ADDR + 0xf4);
				
				fail_flag =1;
				data_fail_num ++;
				if (print_switch) debug("Controller data FAIL at round %d\n", r);
				if (print_switch) debug("expecteded data is 0x%8x%8x,\nfailed data is 0x%8x%8x,\nfailed addr is 0x%8x%8x\n", 
				        exp_data_h, exp_data_l, fail_data_h, fail_data_l, fail_addr_h, fail_addr_l);
				data_bist_status = 0;
			}
	  	
			REG_WRITE(DDR_BASE_ADDR + 0xd4, (REG_READ(DDR_BASE_ADDR + 0xd4) & 0xffffff7f | 0x00000080));  //clear the irq
			REG_WRITE(DDR_BASE_ADDR + 0x90, (REG_READ(DDR_BASE_ADDR + 0x90) & 0xf0ffffff | 0x00000000));  //bist stop
		}
		
		if (print_switch) debug("INT_CHK: 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
		REG_WRITE(DDR_BASE_ADDR + 0xd4, REG_READ(DDR_BASE_ADDR + 0xd0));
		if (print_switch) debug("INT_CHK: clear all int, 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
		
		if (print_switch) debug("STATUS_CHK: phy_obs_reg 0x%8x, 0x%8x, 0x%8x, 0x%8x, 0x%8x\n",
		                    REG_READ(DDR_BASE_ADDR+0x214),
		                    REG_READ(DDR_BASE_ADDR+0x254),
		                    REG_READ(DDR_BASE_ADDR+0x294),
		                    REG_READ(DDR_BASE_ADDR+0x2d4),
		                    REG_READ(DDR_BASE_ADDR+0x314));
		
		if (print_switch) debug("STATUS_CHK: dll_obs_reg 0x%8x, 0x%8x, 0x%8x, 0x%8x, 0x%8x\n",
		                    REG_READ(DDR_BASE_ADDR+0x218),
		                    REG_READ(DDR_BASE_ADDR+0x258),
		                    REG_READ(DDR_BASE_ADDR+0x298),
		                    REG_READ(DDR_BASE_ADDR+0x2d8),
		                    REG_READ(DDR_BASE_ADDR+0x318));
		                    

		delay(100);
		//conf_data_l = REG_READ(DRAM_BASE_ADDR+fail_addr_l);	
		//conf_data_h = REG_READ(DRAM_BASE_ADDR+fail_addr_l+4);
		
  	if (fail_flag) 
  	{
  		//debug("confirm failed data is 0x%8x%8x\n", REG_READ(DRAM_BASE_ADDR+fail_addr_l+4), REG_READ(DRAM_BASE_ADDR+fail_addr_l));
  		//debug("confirm failed data is 0x%8x%8x\n", conf_data_h, conf_data_l);
  		break;
  	}
  }
  

} 

void phy_bist(unsigned int i)
{
	unsigned int g_value;
	unsigned int result = 0;
	
	REG_WRITE(DDR_BASE_ADDR+0x20c,0x00220810);
	REG_WRITE(DDR_BASE_ADDR+0x24c,0x00220810);
	REG_WRITE(DDR_BASE_ADDR+0x28c,0x00220810);
	REG_WRITE(DDR_BASE_ADDR+0x2cc,0x00220810);
	REG_WRITE(DDR_BASE_ADDR+0x30c,0x00220810);
	REG_WRITE(DDR_BASE_ADDR+0x344,0x00002800);
	
	REG_WRITE(DDR_BASE_ADDR+0x3c, REG_READ(DDR_BASE_ADDR+0x3c) & 0xff00ffff | 0x00010000);  //af

	//all write lvl delay set to 0
	//REG_WRITE(DDR_BASE_ADDR+0x124, REG_READ(DDR_BASE_ADDR+0x124) & 0xfffffff0 | 0x00000001);
	//REG_WRITE(DDR_BASE_ADDR+0x12c, 0x00000000);
	//REG_WRITE(DDR_BASE_ADDR+0x130, 0x00000000);
	//all read lvl delay set to 0
	REG_WRITE(DDR_BASE_ADDR+0x138, REG_READ(DDR_BASE_ADDR+0x138) & 0xfffff0ff | 0x00000100);
	REG_WRITE(DDR_BASE_ADDR+0x144, REG_READ(DDR_BASE_ADDR+0x144) & 0xff0000ff | 0);
	REG_WRITE(DDR_BASE_ADDR+0x154, REG_READ(DDR_BASE_ADDR+0x154) & 0xffff0000 | 0);
	REG_WRITE(DDR_BASE_ADDR+0x160, REG_READ(DDR_BASE_ADDR+0x160) & 0xff0000ff | 0);
	REG_WRITE(DDR_BASE_ADDR+0x170, REG_READ(DDR_BASE_ADDR+0x170) & 0xffff0000 | 0);
	//for address slice, delay set to 0
	REG_WRITE(DDR_BASE_ADDR+0x340,0x00000000);

	delay(100);

	//set the slave dll delay
	REG_WRITE(DDR_BASE_ADDR+0x210,0x00002000);
	REG_WRITE(DDR_BASE_ADDR+0x250,0x00002000);
	REG_WRITE(DDR_BASE_ADDR+0x290,0x00002000);
	REG_WRITE(DDR_BASE_ADDR+0x2d0,0x00002000);
	REG_WRITE(DDR_BASE_ADDR+0x310,0x00002000);

	REG_WRITE(DDR_BASE_ADDR+0x3c, REG_READ(DDR_BASE_ADDR+0x3c) & 0xff00ffff | 0x00010000);  //af
	
	//internal phy bist
	REG_WRITE(DDR_BASE_ADDR+0x208,0x00024300);
	REG_WRITE(DDR_BASE_ADDR+0x248,0x00024300);
	REG_WRITE(DDR_BASE_ADDR+0x288,0x00024300);
	REG_WRITE(DDR_BASE_ADDR+0x2c8,0x00024300);
	REG_WRITE(DDR_BASE_ADDR+0x308,0x00024300);
	
	//for internal bist, disable all drv & trm
	REG_WRITE(DDR_BASE_ADDR+0x34c, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x350, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x354, 0xffffffff);
	REG_WRITE(DDR_BASE_ADDR+0x358, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x35c, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x360, 0x40004000);
	REG_WRITE(DDR_BASE_ADDR+0x364, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x368, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x36c, 0xffffffff);
	REG_WRITE(DDR_BASE_ADDR+0x370, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x374, 0x00000000);
	REG_WRITE(DDR_BASE_ADDR+0x378, 0x40004000);

	REG_WRITE(DDR_BASE_ADDR+0x3c, REG_READ(DDR_BASE_ADDR+0x3c) & 0xff00ffff | 0x00010000);  //af

  delay(100);
  
	//phy bist start
	REG_WRITE(DDR_BASE_ADDR+0x208,0x00024700);
	REG_WRITE(DDR_BASE_ADDR+0x248,0x00024700);
	REG_WRITE(DDR_BASE_ADDR+0x288,0x00024700);
	REG_WRITE(DDR_BASE_ADDR+0x2c8,0x00024700);
	REG_WRITE(DDR_BASE_ADDR+0x308,0x00024700);
	
	delay(i);
	
	g_value = (REG_READ(DDR_BASE_ADDR+0x214) >> 1) & 0x00000001;
	result = result | g_value;
	g_value = (REG_READ(DDR_BASE_ADDR+0x254) >> 1) & 0x00000001;
	result = result | (g_value << 1);
	g_value = (REG_READ(DDR_BASE_ADDR+0x294) >> 1) & 0x00000001;
	result = result | (g_value << 2);
	g_value = (REG_READ(DDR_BASE_ADDR+0x2d4) >> 1) & 0x00000001;
	result = result | (g_value << 3);
	g_value = (REG_READ(DDR_BASE_ADDR+0x314) >> 1) & 0x00000001;
	result = result | (g_value << 4);

	if (result) 
		{
			debug("PHY bist FAIL for %d instruction cycles!\n", i);
		}
	else
		{
			debug("PHY bist PASS for %d instruction cycles!\n", i);
		}
	
	while (1);
	
	//internal phy bist end
	REG_WRITE(DDR_BASE_ADDR+0x208,0x00024b00);
	REG_WRITE(DDR_BASE_ADDR+0x248,0x00024b00);
	REG_WRITE(DDR_BASE_ADDR+0x288,0x00024b00);
	REG_WRITE(DDR_BASE_ADDR+0x2c8,0x00024b00);
	REG_WRITE(DDR_BASE_ADDR+0x308,0x00024b00);

}

void wr_lvl()
{
	int start_0[4] = {0};
	int start_1[4] = {0};
	int end_0[4] = {0};
	int end_1[4] = {0};
	int start_0_save[4] = {0};
	int start_1_save[4] = {0};
	int end_0_save[4] = {0};
	int end_1_save[4] = {0};
	int start_0_acc[4] = {0};
	int end_0_acc[4] = {192};
	int start_1_acc[4] = {-64};
	int end_1_acc[4] = {128};
	unsigned int seq_0[4] = {0};
	unsigned int seq_1[4] = {0};
	unsigned int r,i,t;
	unsigned int wrlvl_delay;
	unsigned int wr_delay[4] = {0};
	unsigned int swlvl_resp[4];

	REG_WRITE(DDR_BASE_ADDR + 0x120, (REG_READ(DDR_BASE_ADDR + 0x120) & 0xfffff0ff | 0x00000000)); //cs define
	REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xfff0ffff | 0x00010000)); //wr leveling
	REG_WRITE(DDR_BASE_ADDR + 0x118, (REG_READ(DDR_BASE_ADDR + 0x118) & 0xfffffff0 | 0x00000001)); //sw lvl start
	
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
	
	for (t=0; t<10; t++)
	{
		for (r=0; r<128; r++)
		{
			wrlvl_delay = r;
			REG_WRITE(DDR_BASE_ADDR + 0x12c, (REG_READ(DDR_BASE_ADDR + 0x12c) & 0xffff0000 | wrlvl_delay));     //slice 0 delay
			REG_WRITE(DDR_BASE_ADDR + 0x12c, (REG_READ(DDR_BASE_ADDR + 0x12c) & 0x0000ffff | wrlvl_delay<<16)); //slice 1 delay
			REG_WRITE(DDR_BASE_ADDR + 0x130, (REG_READ(DDR_BASE_ADDR + 0x130) & 0xffff0000 | wrlvl_delay));     //slice 2 delay
			REG_WRITE(DDR_BASE_ADDR + 0x130, (REG_READ(DDR_BASE_ADDR + 0x130) & 0x0000ffff | wrlvl_delay<<16)); //slice 3 delay
			
			REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xf0ffffff | 0x01000000)); //sw lvl load
			while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
			
			swlvl_resp[0]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00000001);      //slice 0 leveling result
			swlvl_resp[1]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00000100)>>8;   //slice 1 leveling result
			swlvl_resp[2]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00010000)>>16;  //slice 2 leveling result
			swlvl_resp[3]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x01000000)>>24;  //slice 3 leveling result
			
			//debug ("WRLVL: delay is %d, resp is %d, %d, %d, %d\n", wrlvl_delay, swlvl_resp[0], swlvl_resp[1], swlvl_resp[2], swlvl_resp[3]);
			
			for (i=0; i<4; i++)
			{
				if (!swlvl_resp[i])
				{  //current response is 0
					if (seq_0[i])
					{ //previous response is 0
						end_0[i] = wrlvl_delay;
						if (end_0[i]-start_0[i] > end_0_save[i] - start_0_save[i])
						{
							start_0_save[i] = start_0[i];
							end_0_save[i] = end_0[i];
						}
					}
					else 
					{ //previous response is 1
						start_0[i] = wrlvl_delay;
						end_0[i] = wrlvl_delay;
						seq_1[i] = 0;
						seq_0[i] = 1;
					}
				}
				else 
				{  //current response is 1
					if (seq_1[i])
					{ //previous response is 1
						end_1[i] = wrlvl_delay;
						if (end_1[i]-start_1[i] > end_1_save[i] - start_1_save[i])
						{ 
							start_1_save[i] = start_1[i];
							end_1_save[i] = end_1[i];
						}
					}
					else
					{ //previouse response is 0
						start_1[i] = wrlvl_delay;
						end_1[i] = wrlvl_delay;
						seq_0[i] = 0;
						seq_1[i] = 1;
					}
				}
			}
  	}
  	
  	//debug("WR_LVL: round %d, leveling result is %d, %d, %d, %d\n", 
  	//                        t, start_1_save[0], start_1_save[1], start_1_save[2], start_1_save[3]);

  	for (i=0; i<4; i++)
  	{
  		if (start_0_save[i]>start_0_acc[i]) start_0_acc[i] = start_0_save[i];
  		if (end_0_save[i]<end_0_acc[i]) end_0_acc[i] = end_0_save[i];
  		if (start_1_save[i]>start_1_acc[i]) start_1_acc[i] = start_1_save[i];
  		if (end_1_save[i]<end_1_acc[i]) end_1_acc[i] = end_1_save[i];
  	}
  			
   	
	}

	for (i=0; i<4; i=i+1)
	{
		//if (start_1_save[i] > end_0_save[i] )
		//	wr_delay[i] = ((end_0_save[i] + start_1_save[i])/2)%128;
		//else
		//	wr_delay[i] = ((128 + end_0_save[i] + start_1_save[i])/2)%128;
		wr_delay[i] = start_1_acc[i];
	}
	
	//set the leveling value
	REG_WRITE(DDR_BASE_ADDR + 0x12c, (REG_READ(DDR_BASE_ADDR + 0x12c) & 0xffff0000 | wr_delay[0]));     //slice 0 delay
	REG_WRITE(DDR_BASE_ADDR + 0x12c, (REG_READ(DDR_BASE_ADDR + 0x12c) & 0x0000ffff | wr_delay[1]<<16)); //slice 1 delay
	REG_WRITE(DDR_BASE_ADDR + 0x130, (REG_READ(DDR_BASE_ADDR + 0x130) & 0xffff0000 | wr_delay[2]));     //slice 2 delay
	REG_WRITE(DDR_BASE_ADDR + 0x130, (REG_READ(DDR_BASE_ADDR + 0x130) & 0x0000ffff | wr_delay[3]<<16)); //slice 3 delay

	REG_WRITE(DDR_BASE_ADDR + 0x210, (REG_READ(DDR_BASE_ADDR + 0x210) & 0xffff00ff | ((wr_delay[0]+0x20)%128)<<8)); //slice 0 delay
	REG_WRITE(DDR_BASE_ADDR + 0x250, (REG_READ(DDR_BASE_ADDR + 0x250) & 0xffff00ff | ((wr_delay[1]+0x20)%128)<<8)); //slice 1 delay
	REG_WRITE(DDR_BASE_ADDR + 0x290, (REG_READ(DDR_BASE_ADDR + 0x290) & 0xffff00ff | ((wr_delay[2]+0x20)%128)<<8)); //slice 2 delay
	REG_WRITE(DDR_BASE_ADDR + 0x2d0, (REG_READ(DDR_BASE_ADDR + 0x2d0) & 0xffff00ff | ((wr_delay[3]+0x20)%128)<<8)); //slice 3 delay
	
	debug ("WRLVL: final write lvl delay is %d, %d, %d, %d\n", wr_delay[0], wr_delay[1], wr_delay[2], wr_delay[3]);
	
	REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xf0ffffff | 0x01000000)); //sw lvl load
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done

	REG_WRITE(DDR_BASE_ADDR + 0x118, (REG_READ(DDR_BASE_ADDR + 0x118) & 0xfffff0ff | 0x00000100)); //sw lvl exit
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
	
}

void rd_lvl_chk(
unsigned int start_save[],
unsigned int end_save[],
unsigned int space,
unsigned int chk_times)
{//check whether all controller bist pass in the rd_lvl window
	int start_point=0;
	int end_point=128;
	unsigned int i, r;
	
	REG_WRITE(DDR_BASE_ADDR + 0x138, (REG_READ(DDR_BASE_ADDR + 0x138) & 0xfffff0ff | 0x00000100));  //enable read lvl reg
	
	for (i=0; i<4; i++)
	{
		if (start_save[i]<64 || start_save[i]>128)
			{
				if(start_point < start_save[i]%128 ) 
				{
					start_point = start_save[i]%128;
				}
			}
		if ( (end_save[i]%128) < end_point)
			end_point = end_save[i]%128;
	}	
			
	for (r=32; r<=32; r=r+2)
	{
		REG_WRITE(DDR_BASE_ADDR + 0x144, (REG_READ(DDR_BASE_ADDR + 0x144) & 0xff0000ff | r<<8));  //slice 0 delay
		REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0xffff0000 | r));     //slice 1 delay
		REG_WRITE(DDR_BASE_ADDR + 0x160, (REG_READ(DDR_BASE_ADDR + 0x160) & 0xff0000ff | r<<8));  //slice 2 delay
		REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0xffff0000 | r));     //slice 3 delay
		
		debug("RDLVL check: read dqs delay is %d\n", r);
		ctrl_bist(1, 0, space, chk_times, 0);
		if(fail_flag)
			break;
		
	}
	
	REG_WRITE(DDR_BASE_ADDR + 0x138, (REG_READ(DDR_BASE_ADDR + 0x138) & 0xfffff0ff));  //disable read lvl reg

}

void rd_lvl()
{
	unsigned int start_0[4] = {0};
	unsigned int start_1[4] = {0};
	unsigned int end_0[4] = {0};
	unsigned int end_1[4] = {0};
	unsigned int start_0_save[4] = {0};
	unsigned int start_1_save[4] = {0};
	unsigned int end_0_save[4] = {0};
	unsigned int end_1_save[4] = {0};
	unsigned int seq_0[4] = {0};
	unsigned int seq_1[4] = {0};
	unsigned int r,i;
	unsigned int rdlvl_delay;
	unsigned int rd_delay[4] = {0};
	unsigned int swlvl_resp[4];
	
	unsigned int start_point=0, end_point=128;
	
	REG_WRITE(DDR_BASE_ADDR + 0x134, (REG_READ(DDR_BASE_ADDR + 0x120) & 0xfff0ffff | 0x00000000)); //cs define
	REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xfff0ffff | 0x00020000)); //rd leveling
	REG_WRITE(DDR_BASE_ADDR + 0x134, (REG_READ(DDR_BASE_ADDR + 0x134) & 0xf0ffffff | 0x00000000)); //lvl for rise edge
	REG_WRITE(DDR_BASE_ADDR + 0x118, (REG_READ(DDR_BASE_ADDR + 0x118) & 0xfffffff0 | 0x00000001)); //sw lvl start
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
	
	for (r=0; r<96; r++)
	{
		rdlvl_delay = r*2;
		REG_WRITE(DDR_BASE_ADDR + 0x144, (REG_READ(DDR_BASE_ADDR + 0x144) & 0xff0000ff | rdlvl_delay<<8));  //slice 0 delay
		REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0xffff0000 | rdlvl_delay));     //slice 1 delay
		REG_WRITE(DDR_BASE_ADDR + 0x160, (REG_READ(DDR_BASE_ADDR + 0x160) & 0xff0000ff | rdlvl_delay<<8));  //slice 2 delay
		REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0xffff0000 | rdlvl_delay));     //slice 3 delay
		
		REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xf0ffffff | 0x01000000)); //sw lvl load
		while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
		
		swlvl_resp[0]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00000001);      //slice 0 leveling result
		swlvl_resp[1]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00000100)>>8;   //slice 1 leveling result
		swlvl_resp[2]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00010000)>>16;  //slice 2 leveling result
		swlvl_resp[3]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x01000000)>>24;  //slice 3 leveling result
		
		//debug ("RDLVL: delay is %d, resp is %d, %d, %d, %d\n", rdlvl_delay, swlvl_resp[0], swlvl_resp[1], swlvl_resp[2], swlvl_resp[3]);
		
		for (i=0; i<4; i++)
		{
			if (!swlvl_resp[i])
			{  //current response is 0
				if (seq_0[i])
				{ //previous response is 0
					end_0[i] = rdlvl_delay;
					if (end_0[i]-start_0[i] > end_0_save[i] - start_0_save[i])
					{
						start_0_save[i] = start_0[i];
						end_0_save[i] = end_0[i];
					}
				}
				else 
				{ //previous response is 1
					start_0[i] = rdlvl_delay;
					end_0[i] = rdlvl_delay;
					seq_1[i] = 0;
					seq_0[i] = 1;
				}
			}
			else 
			{  //current response is 1
				if (seq_1[i])
				{ //previous response is 1
					end_1[i] = rdlvl_delay;
					if (end_1[i]-start_1[i] > end_1_save[i] - start_1_save[i])
					{ 
						start_1_save[i] = start_1[i];
						end_1_save[i] = end_1[i];
					}
				}
				else
				{ //previouse response is 0
					start_1[i] = rdlvl_delay;
					end_1[i] = rdlvl_delay;
					seq_0[i] = 0;
					seq_1[i] = 1;
				}
			}
		}
	}
		
	for (i=0; i<4; i=i+1)
	{
		rd_delay[i] = ((end_0_save[i] + start_0_save[i])/2)%128;
	}
	
	//set the leveling value
	REG_WRITE(DDR_BASE_ADDR + 0x144, (REG_READ(DDR_BASE_ADDR + 0x144) & 0xff0000ff | rd_delay[0]<<8));  //slice 0 delay
	REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0xffff0000 | rd_delay[1]));     //slice 1 delay
	REG_WRITE(DDR_BASE_ADDR + 0x160, (REG_READ(DDR_BASE_ADDR + 0x160) & 0xff0000ff | rd_delay[2]<<8));  //slice 2 delay
	REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0xffff0000 | rd_delay[3]));     //slice 3 delay
	
	debug ("RDLVL: read lvl delay is 0x%x, 0x%x, 0x%x, 0x%x\n", rd_delay[0], rd_delay[1], rd_delay[2], rd_delay[3]);
	REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xf0ffffff | 0x01000000)); //sw lvl load
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done

	REG_WRITE(DDR_BASE_ADDR + 0x118, (REG_READ(DDR_BASE_ADDR + 0x118) & 0xfffff0ff | 0x00000100)); //sw lvl exit
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
	
	//rd_lvl_chk(start_0_save, end_0_save, 20, 50);
	
}

void gt_lvl()
{
	unsigned int start_0[4] = {0};
	unsigned int start_1[4] = {0};
	unsigned int end_0[4] = {0};
	unsigned int end_1[4] = {0};
	unsigned int start_0_save[4] = {0};
	unsigned int start_1_save[4] = {0};
	unsigned int end_0_save[4] = {0};
	unsigned int end_1_save[4] = {0};
	unsigned int seq_0[4] = {0};
	unsigned int seq_1[4] = {0};
	unsigned int r,i;
	unsigned int gtlvl_delay[4];
	unsigned int gt_delay[4] = {0};
	unsigned int swlvl_resp[4];
	unsigned int half_cycle_mode;
	unsigned int dll[4];
	unsigned int cycle[4];
	unsigned int ctns_seq_th = 16;  // which is 16/64 cycles
	
	REG_WRITE(DDR_BASE_ADDR + 0x134, (REG_READ(DDR_BASE_ADDR + 0x120) & 0xfff0ffff | 0x00000000)); //cs define
	REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xfff0ffff | 0x00030000)); //gt leveling
	REG_WRITE(DDR_BASE_ADDR + 0x118, (REG_READ(DDR_BASE_ADDR + 0x118) & 0xfffffff0 | 0x00000001)); //sw lvl start
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
	
	half_cycle_mode = (REG_READ(DDR_BASE_ADDR + 0x20c) & 0x0f000000 ) >> 24;
	dll[0] = (REG_READ(DDR_BASE_ADDR + 0x218) & 0x0000ff00 ) >> 8;
	dll[1] = (REG_READ(DDR_BASE_ADDR + 0x258) & 0x0000ff00 ) >> 8;
	dll[2] = (REG_READ(DDR_BASE_ADDR + 0x298) & 0x0000ff00 ) >> 8;
	dll[3] = (REG_READ(DDR_BASE_ADDR + 0x2d8) & 0x0000ff00 ) >> 8;

	for (i=0; i<4; i++)
	{
		if (half_cycle_mode) {cycle[i] = dll[i]*2;}
		else {cycle[i] = dll[i];}
	}
	
	for (r=0; r<96; r++)
	{
		gtlvl_delay[0] = r*cycle[0]/64;
		gtlvl_delay[1] = r*cycle[1]/64;
		gtlvl_delay[2] = r*cycle[2]/64;
		gtlvl_delay[3] = r*cycle[3]/64;
		REG_WRITE(DDR_BASE_ADDR + 0x148, (REG_READ(DDR_BASE_ADDR + 0x148) & 0xffff0000 | gtlvl_delay[0]));     //slice 0 delay
		REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0x0000ffff | gtlvl_delay[1]<<16)); //slice 1 delay
		REG_WRITE(DDR_BASE_ADDR + 0x164, (REG_READ(DDR_BASE_ADDR + 0x164) & 0xffff0000 | gtlvl_delay[2]));     //slice 2 delay
		REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0x0000ffff | gtlvl_delay[3]<<16)); //slice 3 delay
		
		REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xf0ffffff | 0x01000000)); //sw lvl load
		while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done
		
		swlvl_resp[0]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00000001);      //slice 0 leveling result
		swlvl_resp[1]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00000100)>>8;   //slice 1 leveling result
		swlvl_resp[2]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x00010000)>>16;  //slice 2 leveling result
		swlvl_resp[3]=(REG_READ(DDR_BASE_ADDR + 0x11c) & 0x01000000)>>24;  //slice 3 leveling result
		
		debug ("GTLVL: delay is %d %d %d %d, resp is %d, %d, %d, %d\n", 
		         r*cycle[0]/64, r*cycle[1]/64, r*cycle[2]/64, r*cycle[3]/64, 
		        swlvl_resp[0], swlvl_resp[1], swlvl_resp[2], swlvl_resp[3]);
		
		for (i=0; i<4; i++)
		{
			if (!swlvl_resp[i])
			{  //current response is 0
				if (seq_0[i])
				{ //previous response is 0
					end_0[i] = gtlvl_delay[i];
					if (end_0[i]-start_0[i] > ctns_seq_th)
					{
						start_0_save[i] = start_0[i];
						end_0_save[i] = end_0[i];
					}
				}
				else 
				{ //previous response is 1
					start_0[i] = gtlvl_delay[i];
					end_0[i] = gtlvl_delay[i];
					seq_1[i] = 0;
					seq_0[i] = 1;
				}
			}
			else 
			{  //current response is 1
				if (seq_1[i])
				{ //previous response is 1
					end_1[i] = gtlvl_delay[i];
					if (end_1[i]-start_1[i] > ctns_seq_th)
					{ 
						start_1_save[i] = start_1[i];
						end_1_save[i] = end_1[i];
					}
				}
				else
				{ //previouse response is 0
					start_1[i] = gtlvl_delay[i];
					end_1[i] = gtlvl_delay[i];
					seq_0[i] = 0;
					seq_1[i] = 1;
				}
			}
		}
	}

//	#ifdef TEST_VERSION
//	gt_delay[0] = 28;
//	gt_delay[1] = 30;
//	gt_delay[2] = 31;
//	gt_delay[3] = 26;
	
//	#else
	for (i=0; i<4; i=i+1)
	{
		gt_delay[i] = (end_0_save[i] + start_1_save[i])/2 - cycle[i]/2;
		//if ((gt_delay[i]<28) || (gt_delay[i]>44))
		//{
		//	debug("\nFATAL: gt_lvl fail!!!\n\n");
		//	gtlvl_fail_flag = 1;
		//	ddrinit_fail_flag = 1;
		//	ddr_init();
		//}
		//else
		//{
		//	gtlvl_fail_flag = 1;
		//}
		//if (gtlvl_fail_flag)
		//	return;
	}
//	#endif
	
	//set the leveling value
	REG_WRITE(DDR_BASE_ADDR + 0x148, (REG_READ(DDR_BASE_ADDR + 0x148) & 0xffff0000 | gt_delay[0]));     //slice 0 delay
	REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0x0000ffff | gt_delay[1]<<16)); //slice 1 delay
	REG_WRITE(DDR_BASE_ADDR + 0x164, (REG_READ(DDR_BASE_ADDR + 0x164) & 0xffff0000 | gt_delay[2]));     //slice 2 delay
	REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0x0000ffff | gt_delay[3]<<16)); //slice 3 delay

	debug ("GTLVL: dll lock value is %d, %d, %d, %d\n", cycle[0], cycle[1], cycle[2], cycle[3]);
	debug ("GTLVL: gate lvl delay is %d, %d, %d, %d\n", gt_delay[0], gt_delay[1], gt_delay[2], gt_delay[3]);
	
	REG_WRITE(DDR_BASE_ADDR + 0x114, (REG_READ(DDR_BASE_ADDR + 0x114) & 0xf0ffffff | 0x01000000)); //sw lvl load
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done

	REG_WRITE(DDR_BASE_ADDR + 0x118, (REG_READ(DDR_BASE_ADDR + 0x118) & 0xfffff0ff | 0x00000100)); //sw lvl exit
	while (!(REG_READ(DDR_BASE_ADDR + 0x118) & 0x00010000));  //wait done

  //do workaround for dqs glitch bug, i.e. toggle phy_gate_lpbk_ctrl_reg_X[29]
  REG_WRITE(DDR_BASE_ADDR + 0x208, ((REG_READ(DDR_BASE_ADDR + 0x208) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x208) | 0xdfffffff))));
  REG_WRITE(DDR_BASE_ADDR + 0x248, ((REG_READ(DDR_BASE_ADDR + 0x248) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x248) | 0xdfffffff))));
  REG_WRITE(DDR_BASE_ADDR + 0x288, ((REG_READ(DDR_BASE_ADDR + 0x288) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x288) | 0xdfffffff))));
  REG_WRITE(DDR_BASE_ADDR + 0x2c8, ((REG_READ(DDR_BASE_ADDR + 0x2c8) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x2c8) | 0xdfffffff))));
  
  REG_WRITE(DDR_BASE_ADDR + 0x208, ((REG_READ(DDR_BASE_ADDR + 0x208) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x208) | 0xdfffffff))));
  REG_WRITE(DDR_BASE_ADDR + 0x248, ((REG_READ(DDR_BASE_ADDR + 0x248) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x248) | 0xdfffffff))));
  REG_WRITE(DDR_BASE_ADDR + 0x288, ((REG_READ(DDR_BASE_ADDR + 0x288) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x288) | 0xdfffffff))));
  REG_WRITE(DDR_BASE_ADDR + 0x2c8, ((REG_READ(DDR_BASE_ADDR + 0x2c8) & 0xdfffffff) | (!(REG_READ(DDR_BASE_ADDR + 0x2c8) | 0xdfffffff))));

}

void wr_trn()
{
	int pass_start = 0;
	int pass_end = 0;
	int pass_start_save = 0;
	int pass_end_save = 0;
	int wr_delay = 0;
	unsigned int pass_seq = 0;
	
	unsigned int dll_slave_ctrl_reg_addr[4] = {0x210, 0x250, 0x290, 0x2d0};
	unsigned int r,lane;
	
	for (lane=0; lane<4; lane++)
	{
		for (r=0; r<80; r=r+1)
		{
			REG_WRITE(DDR_BASE_ADDR + dll_slave_ctrl_reg_addr[lane], r<<8);
			REG_WRITE(DDR_BASE_ADDR + 0x3c, REG_READ(DRAM_BASE_ADDR + 0x3c) & 0xff00ffff | 0x00010000);  //send auto refrensh command
			
			delay(500);
			
			ctrl_bist(1, 0, 10, 1, 0);
			
			if ( data_bist_status )
			{	
				//debug ("WR_TRAINING: lane %d, at delay of %3d, PASS!\n", lane, r);
				if (!pass_seq)
				{
					pass_start = r;
					pass_end = r;
					pass_seq = 1;
				}
				else
				{
					pass_end = r;
					if (pass_end-pass_start>pass_end_save-pass_start_save)
					{
						pass_start_save = pass_start;
						pass_end_save = pass_end;
					}
				}
			}
			else
			{
				//debug ("WR_TRAINING: lane %d, at delay of %3d, FAIL!\n", lane, r);
				pass_seq = 0;
			}
		}
				
		//debug ("WR_TRAINING: lane %d, address delay window is %d to %d\n", lane, pass_start_save, pass_end_save);
		
		wr_delay=(pass_start_save+pass_end_save)/2;
		REG_WRITE(DDR_BASE_ADDR + dll_slave_ctrl_reg_addr[lane], wr_delay<<8);
		REG_WRITE(DDR_BASE_ADDR + 0x3c, REG_READ(DRAM_BASE_ADDR + 0x3c) & 0xff00ffff | 0x00010000);  //send auto refrensh command
		
		//debug ("WR_TRAINING: lane %d, address delay is %d\n", lane, wr_delay);
		
	}
		
	debug ("WR_TRAINING: final write delay is 0x%x, 0x%x, 0x%x, 0x%x\n", 
	                      (REG_READ(DDR_BASE_ADDR + dll_slave_ctrl_reg_addr[0]) & 0x0000ff00) >>8,
	                      (REG_READ(DDR_BASE_ADDR + dll_slave_ctrl_reg_addr[1]) & 0x0000ff00) >>8,
	                      (REG_READ(DDR_BASE_ADDR + dll_slave_ctrl_reg_addr[2]) & 0x0000ff00) >>8,
	                      (REG_READ(DDR_BASE_ADDR + dll_slave_ctrl_reg_addr[3]) & 0x0000ff00) >>8);

}


void rd_trn()
{
	int pass_start[4] = {0};
	int pass_end[4] = {0};
	int pass_start_save[4] = {0};
	int pass_end_save[4] = {0};
	unsigned int pass_seq[4] = {0};
	int acc_win_left[4] ={0};
	int acc_win_right[4] = {0};
	unsigned int comp_pass;
	
	unsigned int bt, r,i, round;
	int rdlvl_delay;
	int rd_delay[4] = {0};
	int rd_win[4] = {0};
		
	REG_WRITE(DDR_BASE_ADDR + 0x138, (REG_READ(DDR_BASE_ADDR + 0x138) & 0xfffff0ff | 0x00000100));  //enable read lvl reg
	
	for (bt=0; bt<1; bt++)
	{
		REG_WRITE(DRAM_BASE_ADDR + 0xfe0, 0xffffffff & ~(1 << bt));
		REG_WRITE(DRAM_BASE_ADDR + 0xfe8, 0xffffffff & ~(1 << bt));
		REG_WRITE(DRAM_BASE_ADDR + 0xff0, 0xffffffff & ~(1 << bt));
		REG_WRITE(DRAM_BASE_ADDR + 0xff8, 0xffffffff & ~(1 << bt));
		
		REG_WRITE(DRAM_BASE_ADDR + 0xfe4, 0x00000000 | (1 << bt));
		REG_WRITE(DRAM_BASE_ADDR + 0xfec, 0x00000000 | (1 << bt));
		REG_WRITE(DRAM_BASE_ADDR + 0xff4, 0x00000000 | (1 << bt));
		REG_WRITE(DRAM_BASE_ADDR + 0xffc, 0x00000000 | (1 << bt));
	
		debug ("RD_TRAINING: victim bit is %d, pattern is 0x%x\n", bt, 0xffffffff & ~(1 << bt));
		
		for (i=0; i<4; i++)
		{
			acc_win_left[i] = -64;
			acc_win_right[i] = 128;
		}
		
		for (round=0; round<=200; round++)
		{
			for (i=0; i<4; i++)
			{
				pass_start[i] = 0;
				pass_end[i] = 0;
				pass_start_save[i] =0;
				pass_end_save[i] =0;
				pass_seq[i] = 0;
			}
				
			for (r=0; r<128; r++)
			{
				rdlvl_delay = r;
				REG_WRITE(DDR_BASE_ADDR + 0x144, (REG_READ(DDR_BASE_ADDR + 0x144) & 0xff0000ff | rdlvl_delay<<8));  //slice 0 delay
				REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0xffff0000 | rdlvl_delay));     //slice 1 delay
				REG_WRITE(DDR_BASE_ADDR + 0x160, (REG_READ(DDR_BASE_ADDR + 0x160) & 0xff0000ff | rdlvl_delay<<8));  //slice 2 delay
				REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0xffff0000 | rdlvl_delay));     //slice 3 delay
				
				//debug ("RD_TRAINING: delay is %d, resp is ", rdlvl_delay);
				
				for (i=0; i<4; i++)
				{
					comp_pass = (REG_READ(DRAM_BASE_ADDR +  0xff8) & 0xff<< 8*i) == ((0xffffffff & ~(1 << bt)) & (0xff << 8*i ));
					if( comp_pass ) //if data compare pass
						if (pass_seq[i]) //in pass sequence
						{
							pass_end[i] = rdlvl_delay;
							if (pass_end[i] - pass_start[i] > pass_end_save[i] - pass_start_save[i])
							{
								pass_start_save[i] = pass_start[i];
								pass_end_save[i] = pass_end[i];
							}
						}
						else //in fail sequence
						{
							pass_start[i] = rdlvl_delay;
							pass_end[i] = rdlvl_delay;
							pass_seq[i] = 1;
						}
					else  //if data compare fail
						if (pass_seq[i]) //in pass sequence
						{
							pass_seq[i] = 0;
						}
					//debug ("%d     ", comp_pass );
					//debug ("%d->%d     %d                ", pass_start_save[i], pass_end_save[i], pass_seq[i]);
				}
				
				//debug("\n");
			}
			
			for (i=0; i<4; i=i+1)
			{
				rd_delay[i] = ((pass_end_save[i] + pass_start_save[i]) / 2)%128;
				rd_win[i] = (pass_end_save[i] - pass_start_save[i]) / 2;
				if (pass_end_save[i] > 128)
				{// one cycle shift
					if (pass_end_save[i]-128 < acc_win_right[i])		{acc_win_right[i] = pass_end_save[i]-128; }
					if (pass_start_save[i]-128 > acc_win_left[i])   {acc_win_left[i] = pass_start_save[i]-128; }
				}
				else
				{
					if (pass_end_save[i] < acc_win_right[i])		{acc_win_right[i] = pass_end_save[i];  }
					if (pass_start_save[i] > acc_win_left[i])   {acc_win_left[i] = pass_start_save[i]; }
				}
			}
	
			REG_WRITE(DDR_BASE_ADDR + 0x144, (REG_READ(DDR_BASE_ADDR + 0x144) & 0xff0000ff | rd_delay[0]<<8));  //slice 0 delay
			REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0xffff0000 | rd_delay[1]));     //slice 1 delay
			REG_WRITE(DDR_BASE_ADDR + 0x160, (REG_READ(DDR_BASE_ADDR + 0x160) & 0xff0000ff | rd_delay[2]<<8));  //slice 2 delay
			REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0xffff0000 | rd_delay[3]));     //slice 3 delay
	
			//debug ("RD_TRAINING: round %d, window is %d->%d, %d->%d, %d->%d, %d->%d\n", 
			//	                      round, pass_start_save[0], pass_end_save[0], pass_start_save[1], pass_end_save[1], 
			//	                             pass_start_save[2], pass_end_save[2], pass_start_save[3], pass_end_save[3]);
			
			//debug ("RD_TRAINING: round %d\n", round);
			//debug ("RD_TRAINING: read lvl delay is %d, %d, %d, %d\n", rd_delay[0], rd_delay[1], rd_delay[2], rd_delay[3]);
			//debug ("RD_TRAINING: read lvl delay window is %d, %d, %d, %d\n", rd_win[0], rd_win[1], rd_win[2], rd_win[3]);
			
			
			if(round%100==0) debug ("RD_TRAINING: round %d, window is %d->%d, %d->%d, %d->%d, %d->%d\n", 
				                      round, acc_win_left[0], acc_win_right[0], acc_win_left[1], acc_win_right[1], 
				                             acc_win_left[2], acc_win_right[2], acc_win_left[3], acc_win_right[3]);
		}
					
	}
	
	for (i=0; i<4; i++)
	{
		rd_delay[i] = (acc_win_left[i] + acc_win_right[i])/2;
	}
	
	REG_WRITE(DDR_BASE_ADDR + 0x144, (REG_READ(DDR_BASE_ADDR + 0x144) & 0xff0000ff | rd_delay[0]<<8));  //slice 0 delay
	REG_WRITE(DDR_BASE_ADDR + 0x154, (REG_READ(DDR_BASE_ADDR + 0x154) & 0xffff0000 | rd_delay[1]));     //slice 1 delay
	REG_WRITE(DDR_BASE_ADDR + 0x160, (REG_READ(DDR_BASE_ADDR + 0x160) & 0xff0000ff | rd_delay[2]<<8));  //slice 2 delay
	REG_WRITE(DDR_BASE_ADDR + 0x170, (REG_READ(DDR_BASE_ADDR + 0x170) & 0xffff0000 | rd_delay[3]));     //slice 3 delay
	
	debug("RD_TRAINING: final result is %d, %d, %d, %d\n",
	                     rd_delay[0], rd_delay[1], rd_delay[2], rd_delay[3]);

	
	//rd_lvl_chk(pass_start, pass_end, 28, 50);
}

void addr_trn()
{
	int pass_start = 0;
	int pass_end = 0;
	int pass_start_save = 0;
	int pass_end_save = 0;
	int addr_delay = 0;
	unsigned int pass_seq = 0;
	
	unsigned int r;
	
	for (r=0; r<128; r=r+1)
	{
		//REG_WRITE(DDR_BASE_ADDR + 0x310, r | (r<<8) | (r<<16) | (r<<24) );
		REG_WRITE(DDR_BASE_ADDR + 0x310, r<<8);
		REG_WRITE(DDR_BASE_ADDR + 0x3c, REG_READ(DRAM_BASE_ADDR + 0x3c) & 0xff00ffff | 0x00010000);  //send auto refrensh command
		
		delay(500);
		
		ctrl_bist(1, 0, 10, 1, 0);
		
		if ( data_bist_status )
		{	
			debug ("ADDR_TRAINING: at delay of %3d, PASS!\n", r);
			if (!pass_seq)
			{
				pass_start = r;
				pass_end = r;
				pass_seq = 1;
			}
			else
			{
				pass_end = r;
				if (pass_end-pass_start>pass_end_save-pass_start_save)
				{
					pass_start_save = pass_start;
					pass_end_save = pass_end;
				}
			}
		}
		else
		{
			debug ("ADDR_TRAINING: at delay of %3d, FAIL!\n", r);
			pass_seq = 0;
		}
	}
	
	debug ("ADDR_TRAINING: address delay window is %d to %d\n", pass_start_save, pass_end_save);
	
	addr_delay=(pass_start_save+pass_end_save)/2;
	REG_WRITE(DDR_BASE_ADDR + 0x310, addr_delay<<8);
	REG_WRITE(DDR_BASE_ADDR + 0x3c, REG_READ(DRAM_BASE_ADDR + 0x3c) & 0xff00ffff | 0x00010000);  //send auto refrensh command
	
	debug ("ADDR_TRAINING: final address delay is 0x%x\n", addr_delay);
		
}


void enter_srefresh()
{
	REG_WRITE(DDR_BASE_ADDR + 0x54,  (REG_READ(DDR_BASE_ADDR + 0x54) & 0x00ffffff | 0x8a000000) );
}

void exit_srefresh()
{
	REG_WRITE(DDR_BASE_ADDR + 0x54,  (REG_READ(DDR_BASE_ADDR + 0x54) & 0x00ffffff | 0x81000000) );
}



