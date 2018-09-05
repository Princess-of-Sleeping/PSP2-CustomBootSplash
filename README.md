# Custom Boot Splash
This plugin can display Boot Splash on PS Vita / PS TV at boot.<br>
Please convert the image you want to display with [Boot Splash image converter](https://princess-of-sleeping.github.io/Vita-HTML-Tools/boot_splash/index.html) and place it in ur0:tai/<br>

## install
Look for the line marked "load os0:kd/clockgen.skprx" from boot_config.txt.<br>
Add "- load ur0:tai/custom_boot_splash.skprx" one line before that.<br>
The path of the plugin can be freely ok.<br>
<br>
```
- load ur0:tai/custom_boot_splash.skprx

load os0:kd/clockgen.skprx
#load os0:kd/syscon.skprx
#load os0:kd/rtc.skprx
```
