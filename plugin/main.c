/*

	CBS mod by SKGleba
	All Rights Reserved

*/

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

static int ex(const char* filloc){
  int fd;
  fd = ksceIoOpen(filloc, SCE_O_RDONLY, 0777);
  if (fd < 0) return 0;
  ksceIoClose(fd); return 1;
}

static int LoadBootlogoSingle(void) {
	int ret = 1;
	SceIoStat stat;
	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	void *fb_addr = NULL, *gz_addr = NULL;
	int uid, yid, fd;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	fd = ksceIoOpen("ur0:tai/boot_splash.img", SCE_O_RDONLY, 0);
	ksceIoGetstat("ur0:tai/boot_splash.img", &stat);
	
	uid = ksceKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp);
	ksceKernelGetMemBlockBase(uid, (void**)&fb_addr);
	if (stat.st_size < 0x1FE000) {
		yid = ksceKernelAllocMemBlock("r", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
		ksceKernelGetMemBlockBase(yid, (void**)&gz_addr);
		ksceIoRead(fd, (void *)gz_addr, stat.st_size);
		ksceGzipDecompress((void *)fb_addr, 0x1FE000, (void *)gz_addr, NULL);
		ksceKernelFreeMemBlock(yid);
	} else {
		ksceIoRead(fd, (void *)fb_addr, 0x1FE000);
	}
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
	ret = 0;
	return ret;
}

static int banimthread(SceSize args, void *argp) {
	uint32_t cur = 0, max = 0;
	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	void *fb_addr = NULL, *gz_addr = NULL;
	int uid, yid, fd;
	uint32_t csz = 0;
	char rmax[4], rsz[4], flags[4];
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	fb.size        = sizeof(fb);		
	fb.pitch       = 960;
	fb.pixelformat = 0;
	fb.width       = 960;
	fb.height      = 544;
	fd = ksceIoOpen("ur0:tai/boot_animation.img", SCE_O_RDONLY, 0);
	if (fd < 0) {
		ksceKernelExitDeleteThread(0);
		return 1;
	}
	ksceIoRead(fd, rmax, sizeof(rmax));
	max = *(uint32_t *)rmax;
	ksceIoRead(fd, flags, sizeof(flags));
	uid = ksceKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp);
	ksceKernelGetMemBlockBase(uid, (void**)&fb_addr);
	if (flags[1] == 1) {
		yid = ksceKernelAllocMemBlock("h", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
		ksceKernelGetMemBlockBase(yid, (void**)&gz_addr);
	}
	while (ksceKernelSysrootGetShellPid() < 0) {
		if (cur > max && flags[0] > 0) {
			ksceIoLseek(fd, 0, 0);
			ksceIoRead(fd, rmax, sizeof(rmax));
			max = *(uint32_t *)rmax;
			ksceIoRead(fd, flags, sizeof(flags));
			cur = 0;
		} else if (cur > max && flags[0] == 0) {
			ksceKernelFreeMemBlock(uid);
			ksceKernelFreeMemBlock(yid);
			ksceIoClose(fd);
			ksceKernelExitDeleteThread(0);
			return 1;
		}
		ksceIoRead(fd, rsz, sizeof(rsz));
		csz = *(uint32_t *)rsz;
		if (flags[1] == 1) {
			ksceIoRead(fd, gz_addr, csz);
			ksceGzipDecompress((void *)fb_addr, 0x1FE000, (void *)gz_addr, NULL);
		} else {
			ksceIoRead(fd, fb_addr, csz);
		}
		
		ksceKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
		fb.base = fb_addr;
		ksceDisplaySetFrameBuf(&fb, 1);
		ksceDisplayWaitVblankStart();
		cur++;
	}
	ksceKernelFreeMemBlock(uid);
	if (flags[1] == 1) ksceKernelFreeMemBlock(yid);
	ksceIoClose(fd);
	ksceKernelExitDeleteThread(0);
	return 1;
}


void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args){
	
	static char uwu[4] = {0x30, 0x30, 0x30, 0};
	SceUID fd = ksceIoOpen("ur0:tai/cbs_cfg.bin", SCE_O_RDONLY, 0);
	if (fd < 0)
		return SCE_KERNEL_START_FAILED;
	ksceIoRead(fd, uwu, 4);
	ksceIoClose(fd);
	
	if (uwu[0] == 0x31 && uwu[1] == 0x30 && ex("ur0:tai/boot_splash.img") == 1) {
		LoadBootlogoSingle();
	} else if (uwu[0] == 0x30 && uwu[1] == 0x31 && ex("ur0:tai/boot_animation.img") == 1) {
		SceUID athid = ksceKernelCreateThread("b", banimthread, 0x00, 0x1000, 0, 0, 0);
		ksceKernelStartThread(athid, 0, NULL);
	}
	
	if (uwu[2] == 0x31) ksceKernelDelayThread(uwu[3] * 1000 * 1000);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args){
	return SCE_KERNEL_STOP_SUCCESS;
}
