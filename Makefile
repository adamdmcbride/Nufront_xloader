#
# (C) Copyright 2004-2006, Texas Instruments, <www.ti.com>
# Jian Zhang <jzhang@ti.com>
#
# (C) Copyright 2000-2004
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

HOSTARCH := $(shell uname -m | \
	sed -e s/i.86/i386/ \
	    -e s/sun4u/sparc64/ \
	    -e s/arm.*/arm/ \
	    -e s/sa110/arm/ \
	    -e s/powerpc/ppc/ \
	    -e s/macppc/ppc/)

HOSTOS := $(shell uname -s | tr A-Z a-z | \
	    sed -e 's/\(cygwin\).*/cygwin/')

export	HOSTARCH

# Deal with colliding definitions from tcsh etc.
VENDOR=

#########################################################################

TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
export	TOPDIR

ifeq (include/config.mk,$(wildcard include/config.mk))
# load ARCH, BOARD, and CPU configuration
include include/config.mk
export	ARCH CPU BOARD VENDOR
# load other configuration
include $(TOPDIR)/config.mk

#ifndef CROSS_COMPILE
CROSS_COMPILE = arm-none-eabi-
#CROSS_COMPILE = arm-linux-
export	CROSS_COMPILE
#endif

#########################################################################
# X-LOAD objects....order is important (i.e. start must be first)

OBJS  = cpu/$(CPU)/start.o
 

LIBS += board/$(BOARDDIR)/lib$(BOARD).a
LIBS += cpu/$(CPU)/lib$(CPU).a
LIBS += lib/lib$(ARCH).a
#LIBS += fs/fat/libfat.a
#LIBS += disk/libdisk.a
LIBS += drivers/libdrivers.a
LIBS += drivers/ddr/libddr.a
.PHONY : $(LIBS)

# Add GCC lib
PLATFORM_LIBS += -L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc

SUBDIRS	=  
#########################################################################
#########################################################################

ALL = x-load.bin x-load.img System.map

all:		$(ALL)
x-load.img:	x-load
		./add_head x-load x-load.img
 
x-load.bin:	x-load
		$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@
 
x-load:	$(OBJS) $(LIBS) $(LDSCRIPT)
		UNDEF_SYM=`$(OBJDUMP) -x $(LIBS) |sed  -n -e 's/.*\(__u_boot_cmd_.*\)/-u\1/p'|sort|uniq`;\
 		$(LD) $(LDFLAGS) $(OBJS) \
			--start-group $(LIBS) --end-group $(PLATFORM_LIBS) \
			-Map x-load.map -o x-load
 
$(LIBS):
		$(MAKE) -C `dirname $@`

  
System.map:	x-load
		@$(NM) $< | \
		grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | \
		sort > System.map

oneboot:	x-load.bin
		scripts/mkoneboot.sh

#########################################################################
else
all install x-load x-load.srec oneboot depend dep:
	@echo "System not configured - see README" >&2
	@ exit 1
endif

#########################################################################

unconfig:
	rm -f include/config.h include/config.mk

#========================================================================
# ARM
#========================================================================
#########################################################################
ns115_pbx_config :   unconfig
	@./mkconfig $(@:_config=) arm ns115 ns115_pbx
ns115_test_config :   unconfig
	@./mkconfig $(@:_config=) arm ns115 ns115_test
ns115_pad_ref_config :   unconfig
	@./mkconfig $(@:_config=) arm ns115 ns115_pad_ref
nns115_fpga_config :   unconfig
	@./mkconfig $(@:_config=) arm ns115 ns115_fpga
ns115_phone_test_config :   unconfig
	@./mkconfig $(@:_config=) arm ns115 ns115_phone_test
ns115_phone_ref_config :   unconfig
	@./mkconfig $(@:_config=) arm ns115 ns115_phone_ref

#########################################################################

clean:
	find . -type f \
		\( -name 'core' -o -name '*.bak' -o -name '*~' \
		-o -name '*.o'  -o -name '*.a'  \) -print \
		| xargs rm -f
 
clobber:	clean
	find . -type f \
		\( -name .depend -o -name '*.srec' -o -name '*.bin' \) \
		-print \
		| xargs rm -f
	rm -f $(OBJS) *.bak tags TAGS
	rm -fr *.*~
	rm -f x-load x-load.map $(ALL) 
	rm -f include/asm/proc include/asm/arch

mrproper \
distclean:	clobber unconfig

backup:
	F=`basename $(TOPDIR)` ; cd .. ; \
	gtar --force-local -zcvf `date "+$$F-%Y-%m-%d-%T.tar.gz"` $$F

#########################################################################
