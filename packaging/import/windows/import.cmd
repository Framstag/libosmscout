@echo off
SETLOCAL ENABLEEXTENSIONS

rem attion variable scope is lost when nesting if's
rem helpfull for string http://www.dostips.com/DtTipsStringManipulation.php
 
set arg1=%1
set dir=%1
set found=0
set search=%1
set scriptdir=%~dp0

if "%arg1%" == "" (
   echo no sourcefile in commandline
   set arg1=local
)

echo Command line parameter "%arg1%"

IF EXIST "%arg1%-latest.osm.pbf" (
   set arg1=%arg1%-latest.osm.pbf
   set found=1
   goto checkdone
)

IF EXIST "%arg1%.osm.pbf" (
   set arg1=%arg1%.osm.pbf
   set found=1
   goto checkdone
)

IF EXIST "%arg1%.pbf" (
   set arg1=%arg1%.pbf
   set found=1
   goto checkdone
)

IF EXIST "%arg1%.osm" (
   set arg1=%arg1%.osm
   set found=1
   goto checkdone
)

echo try substring from "%arg1%"

IF EXIST "%arg1%" (
   echo file komplett in commandline
   goto test1
)

:checkdone

if "%found%" == "0" (
  echo import file not found
  GOTO ExitNotFound
) else (
  echo starting import with file %arg1% into %dir%
)

IF NOT EXIST "%dir%" (
   mkdir %dir%
)

call %scriptdir%\bin\Import --typefile %scriptdir%\stylesheets\map.ost --delete-temporary-files true --delete-debugging-files true --delete-analysis-files true --delete-report-files true --destinationDirectory %dir% %arg1%

GOTO NoErrorExit

:test1
set search=%search:~-15%
echo Last 15 chars "%search%"
if "%search%" == "-latest.osm.pbf" (
  set found=1
  set dir=%dir:-latest.osm.pbf=%
  goto checkdone
)
set search=%search:~-8%
echo Last 8 chars "%search%"
if "%search%" == ".osm.pbf" ( 
  set found=1
  set dir=%dir:.osm.pbf=%
  goto checkdone
)
set search=%search:~-4%
echo Last 4 chars "%search%"
if "%search%" == ".pbf" ( 
  set found=1
  set dir=%dir:.pbf=%
  goto checkdone
)
if "%search%" == ".osm" ( 
  set found=1
  set dir=%dir:.osm=%
  goto checkdone
)
goto checkdone
	
:ExitNotFound
echo import source not found

:NoErrorExit
pause
