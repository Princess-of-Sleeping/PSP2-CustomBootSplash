# Custom Boot Splash
This plugin can display Boot Splash on PS Vita / PS TV at boot.<br>
Please convert the image you want to display with [Boot Splash image converter](https://princess-of-sleeping.github.io/Vita-HTML-Tools/boot_splash/index.html) and place it in ur0:tai/<br>
<br>
The boot logo that is displayed in the beginning can not be hidden with this plugin.<br>

## install
Look for the line marked "load os0:kd/clockgen.skprx" from boot_config.txt.<br>
Add "- load ur0:tai/custom_boot_splash.skprx" one line before that.<br>
The path of the plugin can be freely ok.<br>
<br>
### To Enso 3.65 users
If Enso 3.65 is installed and boot_config.txt exists in vs0:tai/ and it is not loaded even if plugin path is written to ur0:tai/boot_config.txt, please update Enso to the latest version.<br>
<br>
https://github.com/TheOfficialFloW/enso/releases<br>
<br>
<br>
```
- load ur0:tai/custom_boot_splash.skprx

load os0:kd/clockgen.skprx
#load os0:kd/syscon.skprx
#load os0:kd/rtc.skprx
```
