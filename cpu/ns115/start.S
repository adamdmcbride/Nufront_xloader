
#include <config.h>

.globl _start
.global undefined_irq
.global prefetch_irq
.global SWI_irq
.global data_abort_irq
_start:	
	b	reset
 	bl	undefined_irq     //undefined
	bl	SWI_irq		 //Software interrupts
	bl	prefetch_irq     //prefetch abort
	bl	data_abort_irq     //data abort
	b	.    		   //unused
	b	.		   //IRQ, since we disable I/F
	b	.		   //FIQ


.balign 512
.global _serialno
_serialno:
	.fill 128, 2, 0

/*
 *************************************************************************
 *
 * Startup Code (reset vector)
 *
 *************************************************************************
 */
.globl _bss_start
_bss_start:
	.word __bss_start

.globl _bss_end
_bss_end:
	.word _end

.global _romloader_boot_mode
_romloader_boot_mode:
       .fill 4, 1, 0


reset: 
       ldr r1, [r13, #4]
       ldr r0, =_romloader_boot_mode
       str r1, [r0]

	// set the cpu to SVC32 mode
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0

	//disable MMU
	MRC     p15, 0, r0, c1, c0, 0       // read CP15 register 1
        BIC     r0, r0, #0x1                // clear bit 0
        MCR     p15, 0, r0, c1, c0, 0       // write value back

	// Enable all domains to client mode
	MRC     p15, 0, r0, c3, c0, 0      // Read Domain Access Control RegisteR
        LDR     r0, =0x55555555            // This initialize every domain entry to b01 (client)
        MCR     p15, 0, r0, c3, c0, 0      // Write Domain Access Control Register
#if CFG_NS115_PAD_REF
	LDR	r0, =0x06140004
	LDR 	r1, [r0]
	ORR	r1,r1,#0x1;
	STR	r1,[r0]

	LDR	r0, =0x06140000
	LDR 	r1, [r0]
	ORR	r1,r1,#0x1;
	STR	r1,[r0]
#endif
	// Invalidate instruction cache, also flushes TLB	

 //===================================================================        
// Invalidate instruction cache
//===================================================================     
        MOV	r0, #0			// SBZ
        MCR	p15, 0, r0, c7, c5, 0	// Invalidate all instruction caches
        
//===================================================================        
// Invalidate data cache
//===================================================================     
	MOV	r0, #0
	MCR	p15, 2, r0, c0, c0, 0	// Cache size selection register, select data cache
        MRC	p15, 1, r3, c0, c0, 0	// Read cache size from the Cache Size Identification Register
        ldr	r1, =0x1FF
        AND	r3, r1, r3, LSR #13	// r3 = (number of sets - 1)
        MOV	r0, #0			// r0 -> way counter
way_loop:
        MOV	r1, #0			// r1 -> set counter
set_loop:
        MOV	r2, r0, LSL #30		//
        ORR	r2, r1, LSL #5		// r2 -> set/way cache-op format
        MCR	p15, 0, r2, c7, c6, 2	// Invalidate line described by r2
        ADD	r1, r1, #1		// Increment set counter
        CMP	r1, r3			// Check if the last set is reached...
        BLE	set_loop		// ...if not, continue the set_loop...
        ADD	r0, r0, #1		// ...else, Increment way counter
        CMP	r0, #4			// Check if the last way is reached...
        BLT	way_loop		// ...if not, continue the way_loop
   
//===================================================================        
// Invalidate entire Unified main TLB
//===================================================================
	MCR	p15, 0, r0, c8, c7, 0	// (Note: r0 value is ignored)
        

//	MRC	p15, 0, r0, c1, c0, 0	// Read System Control Register
//	ORR	r0, r0, #(0x1 <<12)	// enable I Cache
//	ORR	r0, r0, #(0x1 <<11)	// enable program flow prediction
//	MCR	p15, 0, r0, c1, c0, 0	// Write System Control Register

clear_bss:
	ldr	r0, _bss_start		/* find start of bss segment        */
	ldr	r1, _bss_end		/* stop here                        */
	mov 	r2, #0x00000000		/* clear value                      */
clbss_l:
	str	r2, [r0]		/* clear BSS location               */
	cmp	r0, r1			/* are we at the end yet            */
	add	r0, r0, #4		/* increment clear index pointer    */
	bne	clbss_l                 /* keep clearing till at end        */

	mov	sp,#0			/* set SP to top of IRAM	    */
	movt    sp,#CFG_STACK_ADDR_H	
#	ldr     sp,=CFG_STACK_ADDR

	ldr	pc, _start_armboot	/* jump to C code                   */

_start_armboot:	.word start_armboot

