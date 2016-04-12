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
  bash -lc "pacman -S --needed --noconfirm --sync pacman-mirrors"
  bash -lc "pacman -S --needed --noconfirm git"
  
  IF %BUILDTOOL%==autoconf (
  bash -lc "pacman -S --needed --noconfirm git autoconf automake make"
  ) ELSE (
  bash -lc "pacman -S --needed --noconfirm git make mingw-w64-x86_64-cmake mingw-w64-x86_64-extra-cmake-modules"
  )
  
  bash -lc "pacman -S --needed --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-libtool mingw-w64-x86_64-protobuf mingw-w64-x86_64-libxml2 mingw-w64-x86_64-cairo mingw-w64-x86_64-pango mingw-w64-x86_64-qt5"
)

IF %COMPILER%==msvc2015 (
  cinst wget -x86
  cinst 7zip.commandline -x86

  IF %PLATFORM%==x64 (
    wget -q ftp://ftp.zlatkovic.com/libxml/64bit/zlib-1.2.8-win32-x86_64.7z -O zlib-1.2.8-win32-x86_64.7z
    wget -q ftp://ftp.zlatkovic.com/libxml/64bit/iconv-1.14-win32-x86_64.7z -O iconv-1.14-win32-x86_64.7z
    wget -q ftp://ftp.zlatkovic.com/libxml/64bit/libxml2-2.9.3-win32-x86_64.7z -O libxml2-2.9.3-win32-x86_64.7z

    7z x zlib-1.2.8-win32-x86_64.7z -ozlib -y > nul
    7z x iconv-1.14-win32-x86_64.7z -oiconv -y > nul
    7z x libxml2-2.9.3-win32-x86_64.7z -olibxml2 -y > nul
  )
)
