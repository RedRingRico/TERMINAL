@echo off

echo %PATH%

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

git rev-parse --abbrev-ref HEAD > git_branch
SET /P GITBRANCH=<git_branch
del git_branch

echo Generating Git Version Header for %2
echo Revision: %GITMAJOR%.%GITMINOR%.%GITREVISION%.%GITBUILD% [%GITHASH%] %GITDATE%

SETLOCAL ENABLEDELAYEDEXPANSION

set INPUT=%2
CALL :UCase INPUT PROJECT
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

GOTO:EOF

:LCase
:UCase
:: Converts to upper/lower case variable contents
:: Syntax: CALL :UCase _VAR1 _VAR2
:: Syntax: CALL :LCase _VAR1 _VAR2
:: _VAR1 = Variable NAME whose VALUE is to be converted to upper/lower case
:: _VAR2 = NAME of variable to hold the converted value
:: Note: Use variable NAMES in the CALL, not values (pass "by reference")

SET _UCase=A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
SET _LCase=a b c d e f g h i j k l m n o p q r s t u v w x y z
SET _Lib_UCase_Tmp=!%1!
IF /I "%0"==":UCase" SET _Abet=%_UCase%
IF /I "%0"==":LCase" SET _Abet=%_LCase%
FOR %%Z IN (%_Abet%) DO SET _Lib_UCase_Tmp=!_Lib_UCase_Tmp:%%Z=%%Z!
SET %2=%_Lib_UCase_Tmp%
GOTO:EOF
