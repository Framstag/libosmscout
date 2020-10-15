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

  IF %BUILDTOOL%==meson (
    echo Using build tool 'meson'...
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && meson.exe debug -DenableMapCairo=false && cd debug && ninja"
    echo Finished mason build
  )

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

IF %COMPILER%==msvc2019 (
  @echo on
  echo Compiling libosmscout using Visual Studio 2019...

  echo Initializing Visual Studio command line build environment
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

  IF %BUILDTOOL%==cmake (
    echo Using build tool 'cmake'...
    mkdir build
    cd build
    cmake -G "Visual Studio 16 2019" -A x64 .. -DOSMSCOUT_BUILD_DOC_API=OFF -DCMAKE_SYSTEM_VERSION=10.0.18362.0  -DCMAKE_TOOLCHAIN_FILE=c:\tools\vcpkg\scripts\buildsystems\vcpkg.cmake -Wno-dev
    cmake --build .
    echo Finished cmake build
  )

  IF %BUILDTOOL%==meson (
    echo Using build tool 'meson'...
    mkdir debug
    meson debug --backend vs2019
    cd debug
    msbuild.exe libosmscout.sln /t:build /p:Configuration=debugoptimized /p:Platform="x64"
    echo Finished meson build
  )
)
