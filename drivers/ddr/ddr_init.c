#include "dw_ddr.h"
#include "common.h"

extern unsigned int busfreq;
extern unsigned int pu_code;
extern unsigned int pd_code;

void ddr_sel()
{
	if (busfreq == 200){
#ifdef CFG_NS115_PHONE_TEST
		jedec_lpddr2s4_4gb_x32_1066_freq200_cl5_bl8();
#endif
#ifdef CFG_NS115_PHONE_REF
	
#endif
	}else if(busfreq == 333){
#ifdef CFG_NS115_PAD_REF
        //	work_mode = 0x01210158;
		k4b2g0846d_k0_cs0_freq333_cl5_bl8();
#endif
#ifdef CFG_NS115_TEST
	//	work_mode = 0x01091158;
		k4b2g1646c_h9_freq333_cl5_bl8();	//configuration for k4b2g1646c_h9, frequency is 333
	//	work_mode = 0x03091168;
//		edj2116debg_dj_freq333_cl6_bl8();	//configuration for edj2116debg_dj, frequency is 333,
	//	work_mode = 0x00161158; 
//		mt41j256m16_125_cs1_freq333_cl5_bl8();
	//	work_mode = 0x00121158;
//		mt41j256m16_125_cs0_freq333_cl5_bl8();

#endif
#ifdef CFG_NS115_PHONE_REF
	
#endif
#ifdef CFG_NS115_PHONE_TEST
		jedec_lpddr2s4_4gb_x32_1066_freq333_cl5_bl8();
#endif
	}else if(busfreq == 400){
#ifdef CFG_NS115_PAD_REF
	//	work_mode = 0x01210159;
		k4b2g0846d_k0_cs0_freq400_cl6_bl8();
#endif
#ifdef CFG_NS115_TEST
	//	work_mode = 0x01091068;
		k4b2g1646c_h9_freq400_cl6_bl8();
	//	work_mode = 0x03091068;
//		edj2116debg_dj_freq400_cl6_bl8();
	//	work_mode = 0x00161068;
//		mt41j256m16_125_cs1_freq400_cl6_bl8();
	//	work_mode = 0x00121068;
//		mt41j256m16_125_cs0_freq400_cl6_bl8();
		
#endif
#ifdef CFG_NS115_PHONE_REF
		
#endif
#ifdef CFG_NS115_PHONE_TEST
		
#endif
	}
}


unsigned int workmode_sel()
{	
	unsigned int work_mode;
	if (busfreq == 200){
#ifdef CFG_NS115_PHONE_TEST
		work_mode = 0x141a2258;
#endif
	}else if(busfreq == 333){
#ifdef CFG_NS115_PAD_REF
        	work_mode = 0x01210158;
	//	k4b2g0846d_k0_cs0_freq333_cl5_bl8();
#endif
#ifdef CFG_NS115_TEST
		work_mode = 0x01091158;
	//	k4b2g1646c_h9_freq333_cl5_bl8();
//		work_mode = 0x03091168;
	//	edj2116debg_dj_freq333_cl6_bl8();
//		work_mode = 0x00161158; 
	//	mt41j256m16_125_cs1_freq333_cl5_bl8();
//		work_mode = 0x00121158;
	//	mt41j256m16_125_cs0_freq333_cl5_bl8();

#endif
#ifdef CFG_NS115_PHONE_REF
	
#endif
#ifdef CFG_NS115_PHONE_TEST
		work_mode = 0x141a2158;
#endif
	}else if(busfreq == 400){
#ifdef CFG_NS115_PAD_REF
		work_mode = 0x01210068;
//		k4b2g0846d_k0_cs0_freq400_cl6_bl8();
#endif
#ifdef CFG_NS115_TEST
		work_mode = 0x01091068;
	//	k4b2g1646c_h9_freq400_cl6_bl8();
//		work_mode = 0x03091068;
	//	edj2116debg_dj_freq400_cl6_bl8();
//		work_mode = 0x00161068;
	//	mt41j256m16_125_cs1_freq400_cl6_bl8();
//		work_mode = 0x00121068;
	//	mt41j256m16_125_cs0_freq400_cl6_bl8();
		
#endif
#ifdef CFG_NS115_PHONE_REF
		
#endif
#ifdef CFG_NS115_PHONE_TEST
		
#endif
	}
	return work_mode;
}

void ddr_init()
{
        unsigned int work_mode;
	unsigned int dram_type;
	
        work_mode = workmode_sel();
        debug("ddr init, work mode=0x%x\n",work_mode);
	dram_type = work_mode >> 28;
	
	ddr_reset();
	
	arbiter_init(work_mode);
	
	//integrator should change it according to dram device
	ddr_sel();	
	global_reg_redefine(work_mode);
	
	ddr3_workaround(work_mode);
	
	if (dram_type == 0) 
	{
		pu_code = 49; 
		pd_code = 49;
	}
	else if (dram_type == 1) 
	{
		pu_code = 51; 
		pd_code = 51;
	}
	else if (dram_type == 2) 
	{
		pu_code = 50; 
		pd_code = 50;
	}
	
	zq_cal(work_mode);  // get new pu_code and pd_code
	
	io_init(work_mode);
	
	if (dram_type !=1)
	{//ddr3 or ddr3l mode
		mrs(0, 1, 0x44);
	}
	else
	{//lpddr2 mode
		mrs(1, 3, 0x4);
	}
		
	if (dram_type != 1) 
	{//ddr3 or ddr3l mode
		REG_WRITE(DDR_BASE_ADDR + 0x0, 0x20410601);
	}
	else 
	{//lpddr2 mode
		REG_WRITE(DDR_BASE_ADDR + 0x0, 0x20410501);
	}
		
	while(!(REG_READ(DDR_BASE_ADDR+0xd0) & 0x00000020));
	
	//phy_bist(100);
	
	//if (dram_type == 1) 
	//{
	//	lpddr_drv();
	//}
	
	debug("INT_CHK: 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
	REG_WRITE(DDR_BASE_ADDR + 0xd4, REG_READ(DDR_BASE_ADDR + 0xd0));
	debug("INT_CHK: clear all int, 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
	
	if (dram_type != 1)
	{// if ddr3, do write leveling
//		wr_lvl();
//		wr_trn();
//		gt_lvl();
		//rd_lvl();
//		rd_trn();
		//addr_trn();
	}
	else
	{
		rd_trn();
		wr_trn();
	}

	debug("INT_CHK: 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
	REG_WRITE(DDR_BASE_ADDR + 0xd4, REG_READ(DDR_BASE_ADDR + 0xd0));
	debug("INT_CHK: clear all int, 0x%x\n", REG_READ(DDR_BASE_ADDR + 0xd0));
	
	debug("\n\n");
	
}


