# CBS
This tool can display a Image/Animation on PS Vita at boot.<br>
<br>
The boot logo that is displayed at the beginning can not be hidden with this plugin. Refer to https://github.com/skgleba/enso_ex<br>
<br>
### To update365 users
If Enso 3.65-update365 is installed and boot_config.txt exists in vs0:tai/ CBS will not be loaded, please update Enso to the latest version.<br>
<br>
https://github.com/TheOfficialFloW/enso/releases<br>
<br>

# Functions usage
 - BootLogo:
   - location: ur0:tai/boot_splash.img
   - file format: raw (rgba) or gzipped raw (rgba) 960x544 image.
 - BootAnimation:
   - location: ur0:tai/boot_animation.img
   - file format: custom (see https://github.com/SKGleba/enso_ex/tree/master/cbanim).
 - Delay (5/10/15 sec) - delay at boot, useful to test out the animation.
 
# Notes
 - This is a mod/extension of https://github.com/Princess-of-Sleeping/PSP2-CustomBootSplash, all credits go to PrincessOfSleeping for their awesome tool.