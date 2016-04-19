@echo off

for /f "tokens=1" %%x in ('ver') do set WINVER=%%x
set WINVER=%WINVER:Version =%

FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse HEAD`) DO (
SET GITHASH=%%F
)

FOR /F "tokens=* USEBACKQ" %%F IN ( `git rev-parse --short HEAD` ) DO (
SET GITHASHSHORT=%%F
)

git log -1 --format=%%cd > git_date
SET /P GITDATE=<git_date
del git_date

git describe --tags | sed -e "s/_[0-9].*//" > git_tag
SET /P TAGNAME=<git_tag
del git_tag

git describe --match "%TAGNAME%_[0-9]*" HEAD | sed -e "s/-g.*//" -e "s/%TAGNAME%_//" > git_version_full
SET /P GITVERSION=<git_version_full
del git_version_full

echo %GITVERSION%| sed "s/-[^.]*$//" | sed -r "s/.[^.]*$//" | sed -r "s/.[^.]*$//" > git_major
SET /P GITMAJOR=<git_major
del git_major

echo %GITVERSION%| sed "s/-[^.]*$//" | sed -r "s/.[^.]*$//" | sed -r "s/.[.]*//" > git_minor
SET /P GITMINOR=<git_minor
del git_minor

echo %GITVERSION%| sed "s/-[^.]*$//" | sed -r "s/.*(.[0-9].)//" > git_revision
SET /P GITREVISION=<git_revision
del git_revision

echo %GITVERSION%| sed -e "s/[0-9].[0-9].[0-9]//" -e "s/-//" > git_build
SET /P GITBUILD=<git_build
del git_build

IF "%GITBUILD%"=="" SET GITBUILD=0

git rev-parse --abbrev-ref HEAD > git_branch
SET /P GITBRANCH=<git_branch
del git_branch

echo Generating Git Version Header for %2
echo Revision: %GITMAJOR%.%GITMINOR%.%GITREVISION%.%GITBUILD% [%GITHASH%] %GITDATE%

SETLOCAL ENABLEDELAYEDEXPANSION

set INPUT=%2

IF /I "%WINVER%"=="Microsoft" (
	CALL case_microsoft.bat UCase INPUT PROJECT
)

IF /I "%WINVER%"=="Wine" (
	CALL case_wine.bat UCase INPUT PROJECT
)
SET GITHEADER=%1

echo #ifndef __%PROJECT%_GITVERSION_H__ > %GITHEADER%
echo #define __%PROJECT%_GITVERSION_H__ >> %GITHEADER%
echo. >> %GITHEADER%
echo #define GIT_COMMITHASH      "%GITHASH%" >> %GITHEADER%
echo #define GIT_COMMITHASHSHORT "%GITHASHSHORT%" >> %GITHEADER%
echo #define GIT_COMMITTERDATE   "%GITDATE%" >> %GITHEADER%
echo #define GIT_MAJOR           %GITMAJOR% >> %GITHEADER%
echo #define GIT_MINOR           %GITMINOR% >> %GITHEADER%
echo #define GIT_REVISION        %GITREVISION% >> %GITHEADER%
echo #define GIT_BUILD           %GITBUILD% >> %GITHEADER%
echo #define GIT_VERSION         "%GITMAJOR%.%GITMINOR%.%GITREVISION%.%GITBUILD%" >> %GITHEADER%
echo #define GIT_TAGNAME         "%TAGNAME%" >> %GITHEADER%
echo #define GIT_BRANCH          "%GITBRANCH%" >> %GITHEADER%
echo. >> %GITHEADER%
echo #endif /* __%PROJECT%_GITVERSION_H__ */ >> %GITHEADER%
echo. >> %GITHEADER%

ENDLOCAL
