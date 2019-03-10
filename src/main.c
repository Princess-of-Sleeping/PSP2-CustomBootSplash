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

int config_util(void) {
	psvDebugScreenClear();
	static char uwu[4] = {0x30, 0x30, 0x30, 0};
	printf("CBS-Mod configurator by SKGleba\n");
	printf("\n1 - Boot Logo/Animation ?\n");
	printf("Options:\n");
	printf("  CROSS      Boot Animation\n");
	printf("  SQUARE     Boot Logo\n\n");

	if (get_key() == SCE_CTRL_CROSS) uwu[1] = 0x31;
	
	printf("\n2 - Delay boot ?\n");
	printf("Options:\n");
	printf("  CROSS      Disable\n");
	printf("  SQUARE     5 sec\n");
	printf("  TRIANGLE   10 sec\n");
	printf("  CIRCLE     15 sec\n\n");

	int kii = get_key();
	
	if (kii == SCE_CTRL_SQUARE) {
		uwu[2] = 0x31;
		uwu[3] = 5;
	} else if (kii == SCE_CTRL_TRIANGLE) {
		uwu[2] = 0x31;
		uwu[3] = 10;
	} else if (kii == SCE_CTRL_CIRCLE) {
		uwu[2] = 0x31;
		uwu[3] = 15;
	}
	
	SceUID fd = sceIoOpen("ur0:tai/cbs_cfg.bin", SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
	sceIoWrite(fd, uwu, 4);
	sceIoClose(fd);
	
	printf("done\n");
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	psvDebugScreenInit();

	if (ex("ur0:tai/cbs.skprx") == 0) {
		printf("Could not find cbs.skprx in ur0:tai/ !\n");
		if (ex("ur0:tai/custom_boot_splash.skprx") == 1) sceIoRemove("ur0:tai/custom_boot_splash.skprx");
		printf("copying: cbs plugin... ");
		fcp("app0:cbs.skprx", "ur0:tai/cbs.skprx");
		printf("ok!\n");
		printf("copying: stock animation... ");
		fcp("app0:banim.xgif", "ur0:tai/boot_animation.img");
		printf("ok!\n");
		printf("adding entry to ur0:tai/boot_config.txt... ");
		SceUID fd = sceIoOpen("ur0:tai/boot_config.txt", SCE_O_WRONLY, 0777);
		sceIoWrite(fd, (void *)"# CBS\n- load\tur0:tai/cbs.skprx\n\n# ", strlen("# CBS\n- load\tur0:tai/cbs.skprx\n\n# "));
		sceIoClose(fd);
		printf("ok!\n");
	}
	
	config_util();
	
	press_exit();

	return 0;
}
