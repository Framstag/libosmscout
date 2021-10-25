@echo off

cd %APPVEYOR_BUILD_FOLDER%

echo Compiler: %COMPILER%
echo Architecture: %MSYS2_ARCH%
echo Platform: %PLATFORM%
echo MSYS2 directory: %MSYS2_DIR%
echo MSYS2 system: %MSYSTEM%
echo Configuration: %CONFIGURATION%
echo Build tool: %BUILDTOOL%
echo Target: %TARGET%

set timestart=%time%

echo Start updating build dependencies...

IF %COMPILER%==msys2 (
  echo Installing MSYS2 build preconditions...

  @echo on

  echo Extending path to MSYS and MINGW...
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"

  echo Updating pacman...
  bash -lc "pacman --noconfirm -Syuu --overwrite *"

  echo Killing pacman...
  taskkill.exe /F /FI "MODULES eq msys-2.0.dll

  echo Updating system...
  bash -lc "pacman --noconfirm -Syuu --overwrite *"

  echo Installing git...
  bash -lc "pacman --noconfirm -S --needed git"

  IF %BUILDTOOL%==cmake (
    echo Installing cmake build tool...
    bash -lc "pacman --noconfirm -S --needed make mingw-w64-%MSYS2_ARCH%-cmake mingw-w64-%MSYS2_ARCH%-extra-cmake-modules"
  )

  echo Installing build and compile time dependencies...

  IF %TARGET%==importer (
    bash -lc "pacman --noconfirm -S --needed mingw-w64-%MSYS2_ARCH%-toolchain mingw-w64-%MSYS2_ARCH%-libtool mingw-w64-%MSYS2_ARCH%-libiconv mingw-w64-%MSYS2_ARCH%-libxml2 zip"

    cinst wget -x86

    wget https://github.com/rinigus/marisa-trie/archive/0.2.4.zip -O marisa.zip
    7z x marisa.zip
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && cd marisa-trie-0.2.4 && ./configure --disable-shared && make -j2 && make install"

    wget https://github.com/google/protobuf/releases/download/v3.19.0/protobuf-cpp-3.19.0.zip -O protobuf-cpp-3.19.0.zip
    7z x protobuf-cpp-3.19.0.zip
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && cd protobuf-3.19.0 && ./configure --disable-shared && make -j2 && make install"

  )

  echo Finished installing MSYS2 build preconditions
)
