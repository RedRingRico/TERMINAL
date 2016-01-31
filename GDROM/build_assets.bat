@echo off
cd FONTS
bmfont -c WHITERABBIT.bmfc -o WHITERABBIT.FNT
pvrconv -f -t -4 -nm -nvm -out WHITERABBIT.PVR WHITERABBIT_0.tga
cd ..
exit /b 0

