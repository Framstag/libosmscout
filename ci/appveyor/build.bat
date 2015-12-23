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
  bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && make full"
)
