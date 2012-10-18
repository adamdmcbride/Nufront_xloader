#
# (C) Copyright 2000
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

include $(TOPDIR)/config.mk

# CFLAGS += -DET_DEBUG -DDEBUG

LIB	= libddr.a
OBJS =  ddr_init.o ddr_sub.o
ifeq ($(BOARD),ns115_pad_ref)
OBJS += k4b2g0846d_k0_cs0_freq333_cl5_bl8.o k4b2g0846d_k0_cs0_freq400_cl6_bl8.o
endif

ifeq ($(BOARD),ns115_test)
#OBJS += k4b2g1646c_h9_freq333_cl5_bl8.o k4b2g1646c_h9_freq400_cl6_bl8.o
#OBJS += edj2116debg_dj_freq333_cl6_bl8.o edj2116debg_dj_freq333_cl6_bl8.o
#OBJS += mt41j256m16_125_cs1_freq333_cl5_bl8.o mt41j256m16_125_cs1_freq400_cl6_bl8.o
OBJS += mt41j256m16_125_cs0_freq333_cl5_bl8.o mt41j256m16_125_cs0_freq400_cl6_bl8.o
endif

ifeq ($(BOARD),ns115_phone_ref)
OBJS += 
endif

ifeq ($(BOARD),ns115_phone_test)
OBJS +=	jedec_lpddr2s4_4gb_x32_1066_freq200_cl5_bl8.o jedec_lpddr2s4_4gb_x32_1066_freq333_cl5_bl8.o 
endif

all:	$(LIB)

$(LIB):	$(OBJS)
	$(AR) crv $@ $(OBJS)

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
