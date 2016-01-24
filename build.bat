cd Source
make release
cd ..\GDROM
copy ..\Bin\Dreamcast\1ST_READ.BIN .
ipmaker_cmd ip_config.json
copy %KATANA_ROOT%\Submit\Warning\US_warn.da warning.da
%KATANA_ROOT%\Utl\Dev\CDCraft\crfgdc -bld=BUILD.SCR,dsk -benv=imf1
cim2gdi TERMINAL.cim
cd ..

