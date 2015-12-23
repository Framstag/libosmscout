@echo off

cd %APPVEYOR_BUILD_FOLDER%

echo Compiler: %COMPILER%
echo Architecture: %MSYS2_ARCH%
echo MSYS2 directory: %MSYS2_DIR%
echo MSYS2 system: %MSYSTEM%
echo Bits: %BIT%

IF %COMPILER%==msys2 (
  @echo on
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"
  bash -lc "update-core"
  bash -lc "pacman -S --needed --noconfirm git autoconf automake make mingw-w64-x86_64-toolchain mingw-w64-x86_64-libtool mingw-w64-x86_64-protobuf mingw-w64-x86_64-libxml2 mingw-w64-x86_64-cairo mingw-w64-x86_64-pango mingw-w64-x86_64-qt5"
)
