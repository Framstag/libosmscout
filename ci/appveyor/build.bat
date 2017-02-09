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
  echo "Compiling libosmscout using msys2..."
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"

  IF %BUILDTOOL%==autoconf (
    echo "Using build tool 'autoconf'..."
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && make full"
  ) ELSE (
    echo "Using build tool 'cmake'..."
    IF %TARGET%==importer (
      bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && . packaging/import/windows/build_import.sh"
      appveyor PushArtifact build\libosmscout-importer-Windows-x86_64.zip
    ) ELSE (
        bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && . setupMSYS2.sh && exec 0</dev/null && mkdir build && cd build && CXX=g++ CC=gcc cmake -G 'MSYS Makefiles' .. && cmake -LAH .. && make -j2"
    )
  )
)

IF %COMPILER%==msvc2015 (
  @echo on
  echo "Compiling libosmscout using Visual Studio 2015..."

  IF %BUILDTOOL%==msbuild (
    echo "Using build tool 'msbuild'..."

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
  ) ELSE (
    echo "Using build tool 'cmake'..."

    mkdir build
    cd build
    cmake -G "Visual Studio 14 2015 Win64" ..
    cmake --build .
  )
)
