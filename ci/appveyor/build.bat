@echo off

cd %APPVEYOR_BUILD_FOLDER%

echo Compiler: %COMPILER%
echo Architecture: %MSYS2_ARCH%
echo Platform: %PLATFORM%
echo MSYS2 directory: %MSYS2_DIR%
echo MSYS2 system: %MSYSTEM%
echo Configuration: %CONFIGURATION%
echo Buildtool: %BUILDTOOL%
echo Target: %TARGET%

IF %COMPILER%==msys2 (
  @echo on
  echo Compiling libosmscout using msys2...
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"

  IF %BUILDTOOL%==cmake (
    echo Using build tool 'cmake'...
    IF %TARGET%==importer (
      echo Building importer...
      bash -lc "set -x && cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && . packaging/import/windows/build_import.sh"
      appveyor PushArtifact build\libosmscout-importer-Windows-x86_64.zip
    ) ELSE (
      echo Standard cmake build...
      bash -lc "set -x && cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && mkdir build && cd build && CXX=g++ CC=gcc cmake -G 'MSYS Makefiles' .. && cmake -DOSMSCOUT_BUILD_DOC_API=OFF -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -LAH -Wno-dev .. && make -j2"
    )
    echo Finished cmake build
  )
)
