/*

	CBS mod by SKGleba
	Original CBS by Princess Of Sleeping
	All Rights Reserved

*/

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/display.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "blit.h"

#define printf ksceDebugPrintf

#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

typedef struct {
	uint32_t off;
	uint32_t sz;
	uint8_t code;
	uint8_t type;
	uint8_t active;
	uint32_t flags;
	uint16_t unk;
} __attribute__((packed)) partition_t;

typedef struct {
	char magic[0x20];
	uint32_t version;
	uint32_t device_size;
	char unk1[0x28];
	partition_t partitions[0x10];
	char unk2[0x5e];
	char unk3[0x10 * 4];
	uint16_t sig;
} __attribute__((packed)) master_block_t;

const char *part_code(int code) {
	static char *codes[] = {
		"empty",
		"idstorage",
		"slb2",
		"os0",
		"vs0",
		"vd0",
		"tm0",
		"ur0",
		"ux0",
		"gro0",
		"grw0",
		"ud0",
		"sa0",
		"mediaid",
		"pd0",
		"unused"
	};
	return codes[code];
}

const char *part_type(int type) {
	if (type == 6)
		return "FAT16";
	else if (type == 7)
		return "exFAT";
	else if (type == 0xDA)
		return "raw";
	return "unknown";
}

static int ex(const char* filloc){
  int fd;
  fd = ksceIoOpen(filloc, SCE_O_RDONLY, 0777);
  if (fd < 0) return 0;
  ksceIoClose(fd); return 1;
}

// ty flow
void firmware_string(char string[8], unsigned int version) {
  char a = (version >> 24) & 0xf;
  char b = (version >> 20) & 0xf;
  char c = (version >> 16) & 0xf;
  char d = (version >> 12) & 0xf;

  memset(string, 0, 8);
  string[0] = '0' + a;
  string[1] = '.';
  string[2] = '0' + b;
  string[3] = '0' + c;
  string[4] = '\0';

  if (d) {
    string[4] = '0' + d;
    string[5] = '\0';
  }
}

void readNfoEmmc(master_block_t *master) {
	int entries = 0;
	#define WRAP_EMMC(TEXT,...)\
		blit_stringf(550, 30+20*entries++, (TEXT), __VA_ARGS__);	
	blit_set_color(0x0000FF00, 0xFF000000);
	WRAP_EMMC("%s info:", "EMMC");
	blit_set_color(0x00FFFFFF, 0xFF000000);
	WRAP_EMMC(" Size: 0x%X blocks", master->device_size);
	WRAP_EMMC(" Block size: %d bytes", 512); // Always
	WRAP_EMMC(" Converted size: %dMB", (((master->device_size * 512) / 1024) / 1024));
	blit_set_color(0x00FF0000, 0xFF000000);
	WRAP_EMMC("%s partitions:", "EMMC");
	blit_set_color(0x00FFFFFF, 0xFF000000);
	for (size_t i = 0; i < ARRAYSIZE(master->partitions); ++i) {
		partition_t *p = &master->partitions[i];
		  if (p->code != 0) {
			WRAP_EMMC(" %s %s @0x%X", part_type(p->type), part_code(p->code), p->off);
	      }
	}
}

// live boot animation
static int livebanimthread(SceSize args, void *argp) {
	uint32_t cur = 0, max = 0;
	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	SceIoStat stat;
	void *fb_addr = NULL, *gz_addr = NULL;
	int uid, yid, fd;
	uint32_t csz = 0, curpos = 0;
	char rmax[4], rsz[4], flags[4];
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	fb.size        = sizeof(fb);		
	fb.pitch       = 960;
	fb.pixelformat = 0;
	fb.width       = 960;
	fb.height      = 544;
	fd = ksceIoOpen("ur0:tai/lboot_animation.img", SCE_O_RDONLY, 0);
	ksceIoGetstat("ur0:tai/lboot_animation.img", &stat);
	ksceIoRead(fd, rmax, sizeof(rmax));
	max = *(uint32_t *)rmax;
	ksceIoRead(fd, flags, sizeof(flags));
	uid = ksceKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp);
	ksceKernelGetMemBlockBase(uid, (void**)&fb_addr);
	ksceKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
	yid = ksceKernelAllocMemBlock("temp_anim", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
	ksceKernelGetMemBlockBase(yid, (void**)&gz_addr);
	ksceIoLseek(fd, 0, 0);
	ksceIoRead(fd, gz_addr, stat.st_size);
	ksceIoClose(fd);
	fd = ksceIoOpen("ur0:tai/lboot_splash.img", SCE_O_RDONLY, 0);
	if (fd >= 0) {
		SceIoStat stat_img;
		ksceIoGetstat("ur0:tai/lboot_splash.img", &stat_img);
		if (stat_img.st_size < 0x1FE000) {
			void *img_gz_addr = NULL;
			int xid = ksceKernelAllocMemBlock("temp_img", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
			ksceKernelGetMemBlockBase(xid, (void**)&img_gz_addr);
			ksceIoRead(fd, (void *)img_gz_addr, stat_img.st_size);
			ksceGzipDecompress((void *)fb_addr, 0x1FE000, (void *)img_gz_addr, NULL);
			ksceKernelFreeMemBlock(xid);
		} else {
			ksceIoRead(fd, (void *)fb_addr, 0x1FE000);
		}
		ksceIoClose(fd);
	}
	fb.base = fb_addr;
	blit_set_frame_buf(&fb);
	static master_block_t master;
	fd = ksceIoOpen("sdstor0:int-lp-act-entire", SCE_O_RDONLY, 0);
	ksceIoRead(fd, &master, sizeof(master));
	ksceIoClose(fd);
	int entries = 0;
	int sysroot = ksceKernelGetSysbase();
	int kbl_param = *(int *)(sysroot + 0x6c);
	char cur_fw[8], min_fw[8];
	firmware_string(cur_fw, *(uint32_t *)(*(int *)(sysroot + 0x6c) + 4));
	firmware_string(min_fw, *(uint32_t *)(kbl_param + 8));
	#define WRAP(TEXT,...)\
		blit_stringf(10, 30+20*entries++, (TEXT), __VA_ARGS__);	
	blit_set_color(0x0000FF00, 0xFF000000);
	WRAP("%s info:", "Console");
	blit_set_color(0x00FFFFFF, 0xFF000000);
	WRAP(" Cur firmware: %s", cur_fw);
	WRAP(" Min firmware: %s", min_fw);
	WRAP(" Bootloader Rev: %d", *(uint32_t *)(kbl_param + 0xF8));
	WRAP(" Lboot flags: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0x6C));
	WRAP(" Hboot flags: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0xCC));
	WRAP(" Wakeup factor: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0xC4));
	WRAP(" Hardware Info: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0xD4));
	WRAP(" Config: 0x%016" PRIx64, *(uint64_t *)(kbl_param + 0xA0));
	WRAP(" DRAM base (p): 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0x60));
	WRAP(" DRAM size: %d bytes", *(uint32_t *)(kbl_param + 0x64));
	WRAP(" SK enp paddr: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0x80));
	WRAP(" kprx_auth paddr: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0x90));
	WRAP(" SRVK paddr: 0x%08" PRIx32, *(uint32_t *)(kbl_param + 0x98));
	WRAP(" Secure state bit: 0x%X", ((*(unsigned int *)(sysroot + 0x28) ^ 1) & 1));
	WRAP(" Manufacturing mode: %s", ((*(uint32_t *)(kbl_param + 0x6C) & 0x4) != 0) ? "Yes" : "No");
	WRAP(" Release check mode: %s", (ksceKernelCheckDipsw(159) != 0) ? "Development" : "Release");
	WRAP(" PS TV emulation: %s", (ksceKernelCheckDipsw(152) != 0) ? "On" : "Off");
	readNfoEmmc(&master);
	ksceDisplaySetFrameBuf(&fb, 1);
	curpos = (0 + sizeof(flags) + sizeof(rmax));
	while (ksceKernelSysrootGetShellPid() < 0) {
		if (cur > max && flags[0] > 0) {
			curpos = (0 + sizeof(flags) + sizeof(rmax));
			cur = 0;
		} else if (cur > max && flags[0] == 0) {
			ksceKernelFreeMemBlock(uid);
			ksceKernelFreeMemBlock(yid);
			ksceKernelExitDeleteThread(0);
			return 1;
		}
		csz = *(uint32_t *)(gz_addr + curpos);
		curpos = curpos + sizeof(rsz);
		ksceGzipDecompress((void *)(fb_addr + (416 * 960 * 4)), (128 * 960 * 4), (void *)(gz_addr + curpos), NULL);
		curpos = curpos + csz;
		if (flags[3] > 0) ksceKernelCpuDcacheAndL2WritebackInvalidateRange((fb_addr + (416 * 960 * 4)), (128 * 960 * 4));
		ksceDisplayWaitVblankStart();
		cur++;
	}
	ksceKernelFreeMemBlock(yid);
	ksceKernelFreeMemBlock(uid);
	ksceKernelExitDeleteThread(0);
	return 1;
}

// static boot logo
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
		yid = ksceKernelAllocMemBlock("gz", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
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

// boot animation
static int banimthread(SceSize args, void *argp) {
	uint32_t cur = 0, max = 0;
	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	SceIoStat stat;
	void *fb_addr = NULL, *gz_addr = NULL;
	int uid, yid, fd;
	uint32_t csz = 0, curpos = 0;
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
	ksceIoGetstat("ur0:tai/boot_animation.img", &stat);
	ksceIoRead(fd, rmax, sizeof(rmax));
	max = *(uint32_t *)rmax;
	ksceIoRead(fd, flags, sizeof(flags));
	uid = ksceKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp);
	ksceKernelGetMemBlockBase(uid, (void**)&fb_addr);
	ksceKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
	fb.base = fb_addr;
	ksceDisplaySetFrameBuf(&fb, 1);
	if (stat.st_size < 0x200000 && flags[2] == 0) { // optimized for small anims, copy to mem, then loop
		yid = ksceKernelAllocMemBlock("temp", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
		ksceKernelGetMemBlockBase(yid, (void**)&gz_addr);
		ksceIoLseek(fd, 0, 0);
		ksceIoRead(fd, gz_addr, stat.st_size);
		ksceIoClose(fd);
		curpos = (0 + sizeof(flags) + sizeof(rmax));
		while (ksceKernelSysrootGetShellPid() < 0) {
			if (cur > max && flags[0] > 0) {
				curpos = (0 + sizeof(flags) + sizeof(rmax));
				cur = 0;
			} else if (cur > max && flags[0] == 0) {
				ksceKernelFreeMemBlock(uid);
				ksceKernelFreeMemBlock(yid);
				ksceKernelExitDeleteThread(0);
				return 1;
			}
			csz = *(uint32_t *)(gz_addr + curpos);
			curpos = curpos + sizeof(rsz);
			ksceGzipDecompress((void *)fb_addr, 0x1FE000, (void *)(gz_addr + curpos), NULL);
			curpos = curpos + csz;
			if (flags[3] > 0) ksceKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
			ksceDisplayWaitVblankStart();
			cur++;
		}
		ksceKernelFreeMemBlock(yid);
	} else { // big and raw anims, liveread from emmc
		if (flags[1] == 1) {
			yid = ksceKernelAllocMemBlock("gz", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
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
				if (flags[1] == 1) ksceKernelFreeMemBlock(yid);
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
			if (flags[3] > 0) ksceKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
			ksceDisplayWaitVblankStart();
			cur++;
		}
		if (flags[1] == 1) ksceKernelFreeMemBlock(yid);
	}
	ksceKernelFreeMemBlock(uid);
	ksceKernelExitDeleteThread(0);
	return 1;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args){
	
	static char uwu[4] = {0x30, 0x30, 0x30, 0}; // IMGTYPE, ION, DELAY, DELAYTIMESEC
	SceUID fd = ksceIoOpen("ur0:tai/cbs_cfg.bin", SCE_O_RDONLY, 0);
	if (fd < 0)
		return SCE_KERNEL_START_FAILED;
	ksceIoRead(fd, uwu, 4);
	ksceIoClose(fd);
	
	if (uwu[1] != 0x31)
		return SCE_KERNEL_START_FAILED;
	
	if (uwu[0] == 0x31 && ex("ur0:tai/boot_splash.img") == 1) {
		LoadBootlogoSingle();
	} else if (uwu[0] == 0x32 && ex("ur0:tai/boot_animation.img") == 1) {
		SceUID athid = ksceKernelCreateThread("b", banimthread, 0x00, 0x1000, 0, 0, 0);
		ksceKernelStartThread(athid, 0, NULL);
	} else if (uwu[0] == 0x33 && ex("ur0:tai/lboot_animation.img") == 1) { 
		SceUID bthid = ksceKernelCreateThread("l", livebanimthread, 0x00, 0x1000, 0, 0, 0);
		ksceKernelStartThread(bthid, 0, NULL);
	}
	
	if (uwu[2] == 0x31) ksceKernelDelayThread(uwu[3] * 1000 * 1000);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args){
	return SCE_KERNEL_STOP_SUCCESS;
}
