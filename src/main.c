/*
  Custom Boot Splash
  Copyright (C) 2018, Princess of Sleeping
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/display.h>

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args){

	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	SceIoStat stat;

	void *fb_addr = NULL;

	int stat_ret, uid, fd;

	stat_ret = ksceIoGetstat("ur0:tai/boot_splash.bin", &stat);

	if((stat_ret < 0) || ((uint32_t)stat.st_size != 0x1FE000) || (SCE_SO_ISDIR(stat.st_mode) != 0)){
		return SCE_KERNEL_START_SUCCESS;
	}

	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;

	if((uid = ksceKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp)) < 0){
		return SCE_KERNEL_START_SUCCESS;
	}

	ksceKernelGetMemBlockBase(uid, (void**)&fb_addr);

	fd = ksceIoOpen("ur0:tai/boot_splash.bin", SCE_O_RDONLY, 0);
	ksceIoRead(fd, fb_addr, 0x1FE000);
	ksceIoClose(fd);

	ksceKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);

	fb.size        = sizeof(fb);
	fb.base        = fb_addr;
	fb.pitch       = 960;
	fb.pixelformat = 0;
	fb.width       = 960;
	fb.height      = 544;

	ksceDisplaySetFrameBuf(&fb, 1);

	ksceDisplayWaitVblankStart();

	ksceKernelFreeMemBlock(uid);

	ksceKernelDelayThread(3 * 1000 * 1000);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args){
	return SCE_KERNEL_STOP_SUCCESS;
}
