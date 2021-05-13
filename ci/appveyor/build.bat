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
    )
    echo Finished cmake build
  )
)
