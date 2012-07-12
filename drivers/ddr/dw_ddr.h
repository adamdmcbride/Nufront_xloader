#ifndef	_DDR_DREG_H__
#define	_DDR_DREG_H__

#define	DDR_BASE_ADDR	  (0x050e0000)
#define MEMMAX_BASE	    (0x08008000)
#define DRAM_BASE_ADDR  (0x80000000)

#define	REG_WRITE(addr, value)	(*((volatile unsigned int*)(addr))) = (value)
#define REG_READ(addr)  (*((volatile unsigned int*)(addr)))


#endif



