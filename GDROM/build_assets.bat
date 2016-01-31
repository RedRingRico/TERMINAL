@echo off
cd FONTS
bmfont -c TERMINAL.bmfc -o TERMINAL.FNT
pvrconv -f -t -4 -nm -nvm -out TERMINAL.PVR TERMINAL_0.tga
cd ..
exit /b 0

