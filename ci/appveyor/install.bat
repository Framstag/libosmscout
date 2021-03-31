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

  IF %BUILDTOOL%==meson (
    echo Installing ninja and meson build tool...
    bash -lc "pacman --noconfirm -S --needed mingw-w64-%MSYS2_ARCH%-meson mingw-w64-x86_64-ninja"
  )

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

    wget https://github.com/google/protobuf/releases/download/v3.1.0/protobuf-cpp-3.1.0.zip -O protobuf-cpp-3.1.0.zip
    7z x protobuf-cpp-3.1.0.zip
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && cd protobuf-3.1.0 && ./configure --disable-shared && make -j2 && make install"

  ) ELSE (
    echo Installing common dependencies
    bash -lc "pacman -S --needed --noconfirm mingw-w64-%MSYS2_ARCH%-protobuf mingw-w64-%MSYS2_ARCH%-libxml2 mingw-w64-%MSYS2_ARCH%-cairo mingw-w64-%MSYS2_ARCH%-pango mingw-w64-%MSYS2_ARCH%-qt5 mingw-w64-%MSYS2_ARCH%-glew mingw-w64-%MSYS2_ARCH%-glfw mingw-w64-%MSYS2_ARCH%-glm"
  )

  echo Finished installing MSYS2 build preconditions
)

IF %COMPILER%==msvc2019 (
  @echo on
  echo MSVC2019 build...

  IF %BUILDTOOL%==cmake (
    cd c:\tools\vcpkg

    echo Installing zlib, iconv, libxml2...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install zlib:x64-windows libiconv:x64-windows libxml2:x64-windows

    echo Installing protobuf...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install protobuf:x64-windows

    rem transitive
    echo Installing pixman, expat, fontconfig, cairo, harfbuzz, pango...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install pixman:x64-windows expat:x64-windows fontconfig:x64-windows cairo:x64-windows harfbuzz:x64-windows pango:x64-windows

    rem transitive
    echo Installing double-conversion...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install double-conversion:x64-windows

    rem transitive
    echo Installing qt5-modularscripts...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-modularscripts:x64-windows

    rem transitive
    echo Installing openssl-windows, openssl...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install openssl-windows:x64-windows openssl:x64-windows

    rem transitive
    echo Installing libpq...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install libpq:x64-windows

    rem transitive
    echo Installing sqlite3...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install sqlite3:x64-windows

    rem transitive
    echo Installing icu...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install icu:x64-windows

    rem echo Installing qt5-base...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-base:x64-windows

    rem transitive
    rem echo Installing qt5-graphicaleffects...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-graphicaleffects:x64-windows

    rem echo Installing qt5-declarative...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-declarative:x64-windows

    rem transitive
    rem echo Installing qt5-quickcontrols...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-quickcontrols:x64-windows

    rem transitive
    rem echo Installing qt5-speech...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-speech:x64-windows

    rem transitive
    rem echo Installing qt5-charts...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-charts:x64-windows

    rem transitive
    rem echo Installing qt5-datavis3d...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-datavis3d:x64-windows

    rem transitive
    rem echo Installing qt5-multimedia...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-multimedia:x64-windows

    rem transitive
    rem echo Installing qt5-3d...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-3d:x64-windows

    rem transitive
    rem echo Installing qt5-gamepad...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-gamepad:x64-windows

    rem transitive
    rem echo Installing qt5-imageformats...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-imageformats:x64-windows

    rem transitive
    rem echo Installing qt5-networkauth...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-networkauth:x64-windows

    rem transitive
    rem echo Installing qt5-quickcontrols2...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-quickcontrols2:x64-windows

    rem transitive
    rem echo Installing qt5-scxml...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-scxml:x64-windows

    rem transitive
    rem echo Installing qt5-serialport...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-serialport:x64-windows
   
    rem echo Installing qt5-svg...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-svg:x64-windows

    rem transitive
    rem echo Installing qt5-virtualkeyboard...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-virtualkeyboard:x64-windows

    rem transitive
    rem echo Installing qt5-websockets...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-websockets:x64-windows

    rem echo Installing qt5...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5:x64-windows

    rem echo Installing qt5-tools...
    rem %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install qt5-tools:x64-windows

    echo Installing OpenGL...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install opengl:x64-windows

    echo Installing freeglut...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install freeglut:x64-windows

    echo Installing glm...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install glm:x64-windows

    echo Installing glew...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install glew:x64-windows

    echo Installing glfw3...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg install glfw3:x64-windows

    echo  vcpkg integrate install...
    %APPVEYOR_BUILD_FOLDER%\ci\timedexec.bat %timestart% 1800 .\vcpkg integrate install

    cd %APPVEYOR_BUILD_FOLDER%
  )

  IF %BUILDTOOL%==meson (
    echo Installing meson build tool...
    set "PATH=C:\Python36-x64;C:\Python36-x64\Scripts;%PATH%"
    pip.exe install meson
    echo ...done
  )
)
