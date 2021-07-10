#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <psp2/power.h>
#include <taihen.h>
#include <string.h>

#define CONFIG_PATH "ur0:tai/cbs_cfg.bin"

static char cfg[4] = {0x30, 0x3C, 0x3B, 0x33}; // IMGTYPE, ION, DELAY, DELAYTIMESEC

extern unsigned char _binary_theme_settings_xml_start;
extern unsigned char _binary_theme_settings_xml_size;

static SceUID g_hooks[6];

static int logg(void *buffer, int length, const char* logloc, int create)
{
	int fd;
	if (create == 0) {
		fd = sceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND, 6);
	} else if (create == 1) {
		fd = sceIoOpen(logloc, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	} else if (create == 2) {
		fd = sceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 6);
	}
	if (fd < 0)
		return 0;

	sceIoWrite(fd, buffer, length);
	sceIoClose(fd);
	return 1;
}

static void save_config_user(void) {
  SceUID fd;
  fd = sceIoOpen(CONFIG_PATH, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
  if (fd >= 0) {
    sceIoWrite(fd, &cfg, sizeof(cfg));
    sceIoClose(fd);
  }
}

static int load_config_user(void) {
  SceUID fd;
  fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
  if (fd >= 0) {
    sceIoRead(fd, &cfg, sizeof(cfg));
    sceIoClose(fd);
	if (cfg[3] != 0x33) {
		cfg[0] = 0x30, cfg[1] = 0x3C, cfg[2] = 0x3B, cfg[3] = 0x33;
		save_config_user();
	}
	return 0;
  }
  save_config_user();
  return 0;
}

static tai_hook_ref_t g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int *value) {
  if (sceClibStrncmp(category, "/CONFIG/CBSM", 12) == 0) {
    if (value) {
      load_config_user();
      if (sceClibStrncmp(name, "imgt", 4) == 0)
        *value = (cfg[0] - 0x30);
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static tai_hook_ref_t g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrSetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int value) {
  if (sceClibStrncmp(category, "/CONFIG/CBSM", 12) == 0) {
    if (sceClibStrncmp(name, "imgt", 4) == 0)
		cfg[0] = (value + 0x30);
    save_config_user();
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

typedef struct {
  int size;
  const char *name;
  int type;
  int unk;
} SceRegMgrKeysInfo;

static tai_hook_ref_t g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched(const char *category, SceRegMgrKeysInfo *info, int unk) {
  if (sceClibStrncmp(category, "/CONFIG/CBSM", 12) == 0) {
    if (info) {
        info->type = 0x00040000; // type integer
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, category, info, unk);
}

static tai_hook_ref_t g_scePafMiscLoadXmlLayout_SceSettings_hook;
static int scePafMiscLoadXmlLayout_SceSettings_patched(int a1, void *xml_buf, int xml_size, int a4) {
  if (sceClibStrncmp(xml_buf+82, "theme_settings_plugin", 21) == 0) {
    xml_buf = (void *)&_binary_theme_settings_xml_start;
    xml_size = (int)&_binary_theme_settings_xml_size;
  }
  return TAI_CONTINUE(int, g_scePafMiscLoadXmlLayout_SceSettings_hook, a1, xml_buf, xml_size, a4);
}

static SceUID g_system_settings_core_modid = -1;
static tai_hook_ref_t g_sceKernelLoadStartModule_SceSettings_hook;
static SceUID sceKernelLoadStartModule_SceSettings_patched(char *path, SceSize args, void *argp, int flags, SceKernelLMOption *option, int *status) {
  SceUID ret = TAI_CONTINUE(SceUID, g_sceKernelLoadStartModule_SceSettings_hook, path, args, argp, flags, option, status);
  if (ret >= 0 && sceClibStrncmp(path, "vs0:app/NPXS10015/system_settings_core.suprx", 44) == 0) {
    g_system_settings_core_modid = ret;
    g_hooks[2] = taiHookFunctionImport(&g_scePafMiscLoadXmlLayout_SceSettings_hook, 
                                        "SceSettings", 
                                        0x3D643CE8, // ScePafMisc
                                        0x19FE55A8, 
                                        scePafMiscLoadXmlLayout_SceSettings_patched);
    g_hooks[3] = taiHookFunctionImport(&g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x16DDF3DC, 
                                        sceRegMgrGetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[4] = taiHookFunctionImport(&g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0xD72EA399, 
                                        sceRegMgrSetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[5] = taiHookFunctionImport(&g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x58421DD1, 
                                        sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched);
  }
  return ret;
}

static tai_hook_ref_t g_sceKernelStopUnloadModule_SceSettings_hook;
static int sceKernelStopUnloadModule_SceSettings_patched(SceUID modid, SceSize args, void *argp, int flags, SceKernelULMOption *option, int *status) {
  if (modid == g_system_settings_core_modid) {
    g_system_settings_core_modid = -1;
    if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
    if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
  }
  return TAI_CONTINUE(int, g_sceKernelStopUnloadModule_SceSettings_hook, modid, args, argp, flags, option, status);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  sceClibMemset(&cfg, 0, sizeof(cfg));
  load_config_user();
  g_hooks[0] = taiHookFunctionImport(&g_sceKernelLoadStartModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2DCC4AFA, 
                                      sceKernelLoadStartModule_SceSettings_patched);
  g_hooks[1] = taiHookFunctionImport(&g_sceKernelStopUnloadModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2415F8A4, 
                                      sceKernelStopUnloadModule_SceSettings_patched);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  // free hooks that didn't fail
  if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], g_sceKernelLoadStartModule_SceSettings_hook);
  if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], g_sceKernelStopUnloadModule_SceSettings_hook);
  if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
  if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}
