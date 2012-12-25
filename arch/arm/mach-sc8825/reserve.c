/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>
#include <mach/board.h>
#include <linux/memblock.h>

int __init sc8810_pmem_reserve_memblock(void)
{
#if 0//TODO 
	if (memblock_is_region_reserved(SPRD_PMEM_BASE, SPRD_IO_MEM_SIZE))
		return -EBUSY;
	if (memblock_reserve(SPRD_PMEM_BASE, SPRD_IO_MEM_SIZE))
		return -ENOMEM;
#endif 	
	return 0;
}

void __init tiger_reserve(void)
{
	int ret;
	if (ret = sc8810_pmem_reserve_memblock())
		pr_err("Fail to reserve mem for pmem. errno=%d\n", ret);
}
