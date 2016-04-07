@echo off

cd %APPVEYOR_BUILD_FOLDER%

echo Compiler: %COMPILER%
echo Architecture: %MSYS2_ARCH%
echo Platform: %PLATFORM%
echo MSYS2 directory: %MSYS2_DIR%
echo MSYS2 system: %MSYSTEM%
echo Configuration: %CONFIGURATION%
echo Bits: %BIT%

IF %COMPILER%==msys2 (
  @echo on
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"
  IF %BUILDTOOL%==autoconf (
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && make full"
  )
  ELSE (
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && mkdir build && cd build && cmake .. && make"
  )  
)

IF %COMPILER%==msvc2015 (
  @echo on
  IF %BUILDTOOL%==msbuild (
    copy windows\libosmscout\include\osmscout\CoreFeatures.h libosmscout\include\osmscout\CoreFeatures.h 
    copy windows\libosmscout\include\osmscout\private\Config.h  libosmscout\include\osmscout\private\Config.h 
    copy windows\libosmscout\msvc2015_libosmscout.vcxproj libosmscout\libosmscout.vcxproj

    copy windows\libosmscout-import\include\osmscout\ImportFeatures.h libosmscout-import\include\osmscout\ImportFeatures.h 
    copy windows\libosmscout-import\include\osmscout\private\Config.h libosmscout-import\include\osmscout\private\Config.h 
    copy windows\libosmscout-import\msvc2015_libosmscout-import.vcxproj libosmscout-import\libosmscout-import.vcxproj 

    copy windows\libosmscout-map\include\osmscout\MapFeatures.h libosmscout-map\include\osmscout\MapFeatures.h 
    copy windows\libosmscout-map\include\osmscout\private\Config.h  libosmscout-map\include\osmscout\private\Config.h 
    copy windows\libosmscout-map\msvc2015_libosmscout-map.vcxproj libosmscout-map\libosmscout-map.vcxproj 

    copy windows\Import\msvc2015_Import.vcxproj Import\Import.vcxproj

    copy windows\msvc2015_libosmscout.sln libosmscout.sln   

    "C:\Program Files (x86)\MSBuild\14.0\Bin\msbuild.exe" /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM% libosmscout.sln   
  )
  ELSE (
    mkdir build
    cd build
    cmake .. -G "Visual Studio 14 2015 Win64"
    cmake --build .
  )
)
