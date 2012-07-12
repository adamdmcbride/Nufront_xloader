/*
 * (C) Copyright 2004-2006 Texas Insturments
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * CPU specific code
 */
#include <common.h>

unsigned int busfreq,cpufreq;

int cpu_init (void)
{
	unsigned int bus_mode,cpu_mode;
//	unsigned int busfreq,cpufreq;
	int ddr_freq[] = {400,333,266,200};
	int cpu_freq[] = {1200,1000,800,600};

//	unsigned int sysmode = *(unsigned int *)0x0582207c;
	bus_mode = ((*((unsigned int*)0x0582207c)) >> 5) & 0x3;
	cpu_mode = ((*((unsigned int*)0x0582207c)) >> 3) & 0x3;

	busfreq = ddr_freq[bus_mode];
	cpufreq = cpu_freq[cpu_mode];

	printf("cpu freq = %dMHz, bus freq = %dMhz\n",cpufreq,busfreq);
	return 0;
}

void prefetch_irq(void)
{
	TRACE(KERN_ERROR, "Prefetch IRQ\n");
	while(1);
}

void undefined_irq(void)
{
	TRACE(KERN_ERROR, "Undefined IRQ\n");
	while(1);
}

void SWI_irq(void)
{
	TRACE(KERN_ERROR, "SWI IRQ\n");
	while(1);
}

void data_abort_irq(void)
{
	TRACE(KERN_ERROR, "Data abort IRQ\n");
	while(1);
}
