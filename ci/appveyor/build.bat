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

  IF %BUILDTOOL%==autoconf (
    echo Using build tool 'autoconf'...
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && make full"
  )

  IF %BUILDTOOL%==meson (
    echo Using build tool 'meson'...
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && meson.py debug && cd debug && ninja"
  )

  IF %BUILDTOOL%==cmake (
    echo Using build tool 'cmake'...
    IF %TARGET%==importer (
      bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && . packaging/import/windows/build_import.sh"
      appveyor PushArtifact build\libosmscout-importer-Windows-x86_64.zip
    ) ELSE (
        bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && mkdir build && cd build && CXX=g++ CC=gcc cmake -G 'MSYS Makefiles' .. && cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -LAH .. && make -j2"
    )
  )
)

IF %COMPILER%==msvc2015 (
  @echo on
  echo "Compiling libosmscout using Visual Studio 2015..."

  IF %BUILDTOOL%==cmake (
    echo "Using build tool 'cmake'..."
    SET "CMAKE_PREFIX_PATH=C:\Qt\5.8\msvc2015_64"
    mkdir build
    cd build
    cmake -G "Visual Studio 14 2015 Win64" ..
    cmake --build .
  ) ELSE IF %BUILDTOOL%==meson (
    echo "Using build tool 'meson'..."
    SET "PYTHON=C:\Python36-x64"
    DIR %PYTHON%
    SET "PATH=%PYTHON%;%PYTHON%\Scripts;%PATH%"
    echo PYTHON: %PYTHON%
    echo PATH: %PATH%
    mkdir debug
    meson debug --backend vs2015
  )
)
