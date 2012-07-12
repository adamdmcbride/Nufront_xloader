@@@@!
@@@@ \file    sys_dmc_pbx.s
@@@@ \brief   PL340 dynamic memory controller initialisation
@@@@ \date    Copyright (c) 1996-2005 ARM Limited. All rights reserved.
@@@@
@@@@  Provides routine to initialize dynamic memory controller
@@@@

@@@@ Revision $Revision: 23404 $
@@@@ Checkin $Date: 2008-10-24 11:56:05 +0100 (Fri, 24 Oct 2008) $

@@    INCLUDE stdmacros.s     
@@   INCLUDE sys_system.s
        
.global	ddr_init

.equ DMC_BASE        , 0x100E0000 

@@@@ Register definitions for the dynamic memory controller

.equ DMC_STATUS          ,     0x00        @@@@ Status register
.equ DMC_COMMAND         ,     0x04        @@@@ Command register
.equ DMC_DIRECT_CMD      ,     0x08        @@@@ Direct Command register
.equ DMC_MEMORY_CONFIG   ,     0x0C        @@@@ Memory Configuration register

.equ DMC_REFRESH_PRD     ,     0x10        @@@@ Refresh Period register
.equ DMC_CAS_LATENCY     ,     0x14        @@@@ CAS Latency Register
.equ DMC_T_DQSS          ,     0x18        @@@@ t_dqss register
.equ DMC_T_MRD           ,     0x1C        @@@@ t_mrd register

.equ DMC_T_RAS           ,     0x20        @@@@ t_ras register
.equ DMC_T_RC            ,     0x24        @@@@ t_rc register
.equ DMC_T_RCD           ,     0x28        @@@@ t_rcd register
.equ DMC_T_RFC           ,     0x2C        @@@@ t_rfc register

.equ DMC_T_RP            ,     0x30        @@@@ t_rp register
.equ DMC_T_RRD           ,     0x34        @@@@ t_rrd register
.equ DMC_T_WR            ,     0x38        @@@@ t_wr register
.equ DMC_T_WTR           ,     0x3C        @@@@ t_wtr register

.equ DMC_T_XP            ,     0x40        @@@@ t_xp register
.equ DMC_T_XSR           ,     0x44        @@@@ t_xsr register
.equ DMC_T_ESR           ,     0x48        @@@@ t_esr register

.equ DMC_ID_0_CFG        ,     0x100       @@@@ id_cfg registers

.equ DMC_CHIP_0_CFG      ,     0x200       @@@@ chip_cfg 0 registers
.equ DMC_CHIP_1_CFG      ,     0x204       @@@@ chip_cfg 1 registers
.equ DMC_PLL             ,     0x3000

@@@@ -----------------------------------------------------------------------------

@@@@ ***** Warning *****
@@@@ This code is called before any stack has been set up, therefore it is not
@@@@ possible for it to save any registors.  It can use r0-r7, but it must not 
@@@@ use r8-r13.
@@@@ *******************

@@@@ Initializes dynamic memory controller

ddr_init:
        LDR     r0, =DMC_BASE
    
@@@@ set DLL THRU mode
        ADD     r2,r0,#DMC_PLL
        MOV     r1, #0x1
        STR     r1, [r2, #0]
        
@@@@ set config mode
        MOV     r1, #0x4
        STR     r1, [r0, #DMC_COMMAND]
    
@@@@ initialise memory controlller

@@@@ refresh period
        LDR     r1, =0x185
        STR     r1, [r0, #DMC_REFRESH_PRD]

@@@@ cas latency
        MOV     r1, #0x5
        STR     r1, [r0, #DMC_CAS_LATENCY]

@@@@ t_dqss
        MOV     r1, #0x1
        STR     r1, [r0, #DMC_T_DQSS]

@@@@ t_mrd
        MOV     r1, #0x3
        STR     r1, [r0, #DMC_T_MRD]

@@@@ t_ras
        MOV     r1, #0x5
        STR     r1, [r0, #DMC_T_RAS]

@@@@ t_rc
        MOV     r1, #0x8
        STR     r1, [r0, #DMC_T_RC]

@@@@ t_rcd
        MOV     r1, #0x3
        STR     r1, [r0,#DMC_T_RCD]

@@@@ t_rfc
        MOV     r1, #0xA8
        STR     r1, [r0, #DMC_T_RFC]
    
@@@@ t_rp
        MOV     r1, #0x3
        STR     r1, [r0, #DMC_T_RP]

@@@@ t_rrd
        MOV     r1, #0x2
        STR     r1, [r0, #DMC_T_RRD]

@@@@ t_wr
        MOV     r1, #0x2
        STR     r1, [r0, #DMC_T_WR]

@@@@ t_wtr
        MOV     r1, #0x1
        STR     r1, [r0, #DMC_T_WTR]

@@@@ t_xp
        MOV     r1, #0x4
        STR     r1, [r0, #DMC_T_XP]

@@@@ t_xsr
        MOV     r1, #0xC8
        STR     r1, [r0, #DMC_T_XSR]

@@@@ t_esr
        MOV     r1, #0x18
        STR     r1, [r0, #DMC_T_ESR]

@@@@ set memory config
        LDR     r1, =0x00219F93
        STR     r1, [r0, #DMC_MEMORY_CONFIG]

@@@@ initialise external memory chip 0 & 1

@@@@ set chip select for chip 0
        LDR     r1, =0x000070F0
        STR     r1, [r0, #DMC_CHIP_0_CFG]
@@@@ set chip select for chip 1
        LDR     r1, =0x000080F0
        STR     r1, [r0, #DMC_CHIP_1_CFG]

@@@@ delay
        MOV     r1, #0
loop1:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #100
        BLT     loop1       
        
@@@@ send nop [6]
        LDR     r1, =0x000C0000
        STR     r1, [r0, #DMC_DIRECT_CMD]
        STR     r1, [r0, #DMC_DIRECT_CMD]        
        LDR     r1, =0x001C0000
        STR     r1, [r0, #DMC_DIRECT_CMD]
        STR     r1, [r0, #DMC_DIRECT_CMD]  

@@@@ pre-charge all [7]
        MOV     r1, #0x0
        STR     r1, [r0, #DMC_DIRECT_CMD]
        MOV     r1, #0x00100000
        STR     r1, [r0, #DMC_DIRECT_CMD]        

@@@@ wait tRP & nop [8]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]     
        
@@@@ delay
        MOV     r1, #0             
loop2:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop2                

@@@@ set extended mode register [9]
        LDR     r1, =0x00090000
        STR     r1, [r0, #DMC_DIRECT_CMD]
        LDR     r1, =0x00190000
        STR     r1, [r0, #DMC_DIRECT_CMD]

@@@@ wait tMRD & nop [10]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]  

@@@@ delay
        MOV     r1, #0               
loop3:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop3           

@@@@ set mode register [11]
        LDR     r1, =0x00080163
        STR     r1, [r0, #DMC_DIRECT_CMD]
        LDR     r1, =0x00180163
        STR     r1, [r0, #DMC_DIRECT_CMD]    
        
@@@@ delay
        MOV     r1, #0
loop4:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #100
        BLT     loop4
        
@@@@ wait tMRD & nop [12]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]         

@@@@ delay
        MOV     r1, #0        
loop5:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop5           

@@@@ pre-charge all [13]
        MOV     r1, #0x0
        STR     r1, [r0, #DMC_DIRECT_CMD]
        MOV     r1, #0x100000
        STR     r1, [r0, #DMC_DIRECT_CMD]        
        
@@@@ wait tRP & nop [14]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]           
        
@@@@ delay
        MOV     r1, #0        
loop6:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop6         

@@@@ auto-refresh [15]
        MOV     r1, #0x00040000
        STR     r1, [r0, #DMC_DIRECT_CMD]
        MOV     r1, #0x00140000
        STR     r1, [r0, #DMC_DIRECT_CMD]
        
@@@@ wait tRFC & nop [16]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   

@@@@ delay
        MOV     r1, #0                
loop7:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop7          

@@@@ auto-refresh [17]
        MOV     r1, #0x040000
        STR     r1, [r0, #DMC_DIRECT_CMD]
        MOV     r1, #0x140000
        STR     r1, [r0, #DMC_DIRECT_CMD]        
        
@@@@ wait tRFC & nop [18]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD] 

@@@@ delay
        MOV     r1, #0                
loop8:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop8          

@@@@ set mode register [19]
        LDR     r1, =0x00080063
        STR     r1, [r0, #DMC_DIRECT_CMD]
        LDR     r1, =0x00180063
        STR     r1, [r0, #DMC_DIRECT_CMD]        
        
@@@@ wait tMRD & nop [20]
        MOV     r1, #0x0c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]   
        MOV     r1, #0x1c0000
        STR     r1, [r0, #DMC_DIRECT_CMD]    
        
@@@@ delay
        MOV     r1, #0             
loop9:       LDR     r3, [r0, #DMC_STATUS]    @@@@ read status register
        ADD     r1, r1, #1
        CMP     r1, #10
        BLT     loop9   
    

@@@@----------------------------------------    
@@@@ go command
        MOV     r1, #0x0
        STR     r1, [r0, #DMC_COMMAND]

@@@@ wait for ready
loop31:      LDR     r1, [r0,#DMC_STATUS]
        TST     r1,#1
        BEQ     loop31
        
        BX      lr

@@@@ end of file sys_dmc_pbx.s

