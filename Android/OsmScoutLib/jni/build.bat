@ECHO OFF

REM ###################################################################
REM #
REM #  This source is part of the libosmscout library
REM #  Copyright (C) 2010  Tim Teulings
REM #
REM #  This library is free software; you can redistribute it and/or
REM #  modify it under the terms of the GNU Lesser General Public
REM #  License as published by the Free Software Foundation; either
REM #  version 2.1 of the License, or (at your option) any later version.
REM #
REM #  This library is distributed in the hope that it will be useful,
REM #  but WITHOUT ANY WARRANTY; without even the implied warranty of
REM #  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
REM #  Lesser General Public License for more details.
REM #
REM #  You should have received a copy of the GNU Lesser General Public
REM #  License along with this library; if not, write to the Free Software
REM #  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
REM #

REM ###################################################################
REM # Check for the "Config.h" header file

SET SRC_FILE="include\config\Config.h"
SET DST_FILE="..\..\..\libosmscout\include\osmscout\private\Config.h"

FC %SRC_FILE% %DST_FILE% > nul

IF ERRORLEVEL 1 COPY %SRC_FILE% %DST_FILE% /Y


REM ###################################################################
REM # Check for the "CoreFeatures.h" header file

SET SRC_FILE="include\config\CoreFeatures.h"
SET DST_FILE="..\..\..\libosmscout\include\osmscout\CoreFeatures.h"

FC %SRC_FILE% %DST_FILE% > nul

IF ERRORLEVEL 1 COPY %SRC_FILE% %DST_FILE% /Y


REM ###################################################################
REM # Check for the "MapFeatures.h" header file

SET SRC_FILE="include\config\MapFeatures.h"
SET DST_FILE="..\..\..\libosmscout-map\include\osmscout\MapFeatures.h"

FC %SRC_FILE% %DST_FILE% > nul

IF ERRORLEVEL 1 COPY %SRC_FILE% %DST_FILE% /Y


REM ###################################################################
REM # Run the NDK build tool

CALL ndk-build

REM ###################################################################
REM # Copy the GNU STL shared library to the project lib directory

SET SRC_FILE="%ANDROID_NDK_PATH%\sources\cxx-stl\gnu-libstdc++\4.6\libs\armeabi\libgnustl_shared.so"
SET DST_FILE="..\libs\armeabi\libgnustl_shared.so"

ECHO Copying the GNU STL shared library...

COPY %SRC_FILE% %DST_FILE%
