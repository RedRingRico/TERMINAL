@echo off
cd Source
make release
if not errorlevel 0 (
	echo Failed to build the source code
	cd ..
	exit /b 1
)
cd ..\GDROM
copy ..\Bin\Dreamcast\1ST_READ.BIN .
ipmaker_cmd ip_config.json
copy %KATANA_ROOT%\Submit\Warning\US_warn.da warning.da
%KATANA_ROOT%\Utl\Dev\CDCraft\crfgdc -bld=BUILD.SCR,dsk -benv=imf1
cim2gdi TERMINAL.cim
if not errorlevel 0 (
	echo An error occurred when creating the GDI file
	cd ..
	exit /b 1
)
cd ..

