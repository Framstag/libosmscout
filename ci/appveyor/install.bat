@echo off

cd %APPVEYOR_BUILD_FOLDER%

echo Compiler: %COMPILER%
echo Architecture: %MSYS2_ARCH%
echo Platform: %PLATFORM%
echo MSYS2 directory: %MSYS2_DIR%
echo MSYS2 system: %MSYSTEM%
echo Configuration: %CONFIGURATION%
echo Bits: %BIT%

echo Start updating build dependencies...

IF %COMPILER%==msys2 (
  @echo on
  echo MSYS build...
  echo Extending path to MSYS...
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"
  echo "Updating dependencies...
  bash -lc "pacman -S --needed --noconfirm pacman-mirrors"
  bash -lc "pacman -S --needed --noconfirm git"
  
  IF %BUILDTOOL%==autoconf (
  echo Installing autoconf tools...
  bash -lc "pacman -S --needed --noconfirm git autoconf automake make"
  ) ELSE (
  echo Installing cmake tools...
  bash -lc "pacman -S --needed --noconfirm git make mingw-w64-x86_64-cmake mingw-w64-x86_64-extra-cmake-modules"
  )
  
  echo Installing build and compile time dependencies...
  bash -lc "pacman -S --needed --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-libtool mingw-w64-x86_64-protobuf mingw-w64-x86_64-libxml2 mingw-w64-x86_64-cairo mingw-w64-x86_64-pango mingw-w64-x86_64-qt5"
)

IF %COMPILER%==msvc2015 (
  @echo on
  echo MSVC2015 build...
  echo Installing 7zip command line tools...
  
  cinst wget -x86
  cinst 7zip.commandline -x86
  
  echo Adding '%ChocolateyInstall%\lib\7zip.commandline\tools' to path
  set "PATH=%ChocolateyInstall%\lib\7zip.commandline\tools;%PATH%"

  IF %PLATFORM%==x64 (
    wget -q ftp://ftp.zlatkovic.com/libxml/64bit/zlib-1.2.8-win32-x86_64.7z -O zlib-1.2.8-win32-x86_64.7z
    wget -q ftp://ftp.zlatkovic.com/libxml/64bit/iconv-1.14-win32-x86_64.7z -O iconv-1.14-win32-x86_64.7z
    wget -q ftp://ftp.zlatkovic.com/libxml/64bit/libxml2-2.9.3-win32-x86_64.7z -O libxml2-2.9.3-win32-x86_64.7z

    7z x zlib-1.2.8-win32-x86_64.7z -ozlib -y > nul
    7z x iconv-1.14-win32-x86_64.7z -oiconv -y > nul
    7z x libxml2-2.9.3-win32-x86_64.7z -olibxml2 -y > nul
  )
)
