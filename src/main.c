#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/ctrl.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "debug_screen.h"

#define printf psvDebugScreenPrintf

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	PROGRESS_BAR_WIDTH = SCREEN_WIDTH,
	PROGRESS_BAR_HEIGHT = 10,
	LINE_SIZE = SCREEN_WIDTH,
};

static unsigned buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};

int fcp(const char *from, const char *to) {
	long psz;
	FILE *fp = fopen(from,"rb");

	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);

	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);

	FILE *pFile = fopen(to, "wb");
	
	for (int i = 0; i < psz; ++i) {
			fputc(pbf[i], pFile);
	}
   
	fclose(fp);
	fclose(pFile);
	return 1;
}

int fap(const char *from, const char *to) {
	long psz;
	FILE *fp = fopen(from,"rb");

	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);

	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);

	FILE *pFile = fopen(to, "ab");
	
	for (int i = 0; i < psz; ++i) {
			fputc(pbf[i], pFile);
	}
   
	fclose(fp);
	fclose(pFile);
	return 1;
}

uint32_t get_key(void) {
	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned new = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (size_t i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (new & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

void press_exit(void) {
	printf("Press any key to exit this application.\n");
	get_key();
	sceKernelExitProcess(0);
}

int ex(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	psvDebugScreenInit();
	
	static char version[16];
	
	if (ex("ur0:tai/cbs.skprx") == 1)
		sceIoRemove("ur0:tai/cbs.skprx");
	
	if (ex("ur0:tai/boot_config.txt") == 0) {
		printf("Could not find boot_config.txt in ur0:tai/ !\n");
		sceKernelDelayThread(3 * 1000 * 1000);
		sceKernelExitProcess(0);
		sceKernelDelayThread(3 * 1000 * 1000);
	}

	if (ex("ur0:tai/cbsm.skprx") == 0) {
		printf("Could not find cbsm.skprx in ur0:tai/ !\n");
		printf("copying... ");
		fcp("app0:cbsm.skprx", "ur0:tai/cbsm.skprx");
		printf("ok!\n");
		printf("copying: stock animation... ");
		fcp("app0:banim.xgif", "ur0:tai/boot_animation.img");
		printf("ok!\n");
		printf("copying: stock low animation... ");
		fcp("app0:lbanim.xgif", "ur0:tai/lboot_animation.img");
		printf("ok!\n");
		printf("adding entry to ur0:tai/boot_config.txt... ");
		SceUID fd = sceIoOpen("ur0:tai/boot_config_temp.txt", SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
		sceIoWrite(fd, (void *)"# CBSM\n- load\tur0:tai/cbsm.skprx\n\n", strlen("# CBSM\n- load\tur0:tai/cbsm.skprx\n\n"));
		sceIoClose(fd);
		fap("ur0:tai/boot_config.txt", "ur0:tai/boot_config_temp.txt");
		if (ex("ur0:tai/cbsm_old_cfg.txt") == 0) sceIoRename("ur0:tai/boot_config.txt", "ur0:tai/cbsm_old_cfg.txt");
		sceIoRename("ur0:tai/boot_config_temp.txt", "ur0:tai/boot_config.txt");
		printf("ok!\n");
	} else {
		printf("Found cbsm.skprx in ur0:tai/ !\n");
		printf("removing... ");
		sceIoRemove("ur0:tai/cbsm.skprx");
		printf("ok!\n");
		if (ex("ur0:tai/cbsm_old_cfg.txt") == 1) {
			printf("restoring old boot_config.txt... ");
			sceIoRemove("ur0:tai/boot_config.txt");
			sceIoRename("ur0:tai/cbsm_old_cfg.txt", "ur0:tai/boot_config.txt");
			printf("ok!\n");
		}
	}
	
	if (ex("ur0:tai/cbsm.suprx") == 0) {
		printf("Could not find cbsm.suprx in ur0:tai/ !\n");
		printf("copying... ");
		fcp("app0:cbsm.suprx", "ur0:tai/cbsm.suprx");
		printf("ok!\n");
		if (ex("ux0:tai/config.txt") == 1 && ex("ux0:tai/config_precbsm.txt") == 0) sceIoRename("ux0:tai/config.txt", "ux0:tai/config_precbsm.txt");
		printf("adding entry to ur0:tai/config.txt... ");
		SceUID fd = sceIoOpen("ur0:tai/config_temp.txt", SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
		sceIoWrite(fd, (void *)"\n# CBSM\n*NPXS10015\nur0:tai/cbsm.suprx\n", strlen("\n# CBSM\n*NPXS10015\nur0:tai/cbsm.suprx\n"));
		sceIoClose(fd);
		fcp("ur0:tai/config.txt", "ur0:tai/config_precbsm.txt");
		fap("ur0:tai/config_temp.txt", "ur0:tai/config.txt");
		sceIoRemove("ur0:tai/config_temp.txt");
		printf("ok!\n");
	} else {
		printf("Found cbsm.suprx in ur0:tai/ !\n");
		printf("removing... ");
		sceIoRemove("ur0:tai/cbsm.suprx");
		printf("ok!\n");
		if (ex("ur0:tai/config_precbsm.txt") == 1) {
			printf("restoring old config.txt... ");
			sceIoRemove("ur0:tai/config.txt");
			sceIoRename("ur0:tai/config_precbsm.txt", "ur0:tai/config.txt");
			printf("ok!\n");
		}
	}
	
	printf("done!\n");
	sceKernelDelayThread(4 * 1000 * 1000);
	sceKernelExitProcess(0);
}
