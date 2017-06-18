@echo off

cd %APPVEYOR_BUILD_FOLDER%

echo Compiler: %COMPILER%
echo Architecture: %MSYS2_ARCH%
echo Platform: %PLATFORM%
echo MSYS2 directory: %MSYS2_DIR%
echo MSYS2 system: %MSYSTEM%
echo Configuration: %CONFIGURATION%
echo Target: %TARGET%

echo Start updating build dependencies...

IF %COMPILER%==msys2 (
  @echo on
  echo Installtion MSYS2 build preconditions...

  echo Extending path to MSYS...
  SET "PATH=C:\%MSYS2_DIR%\%MSYSTEM%\bin;C:\%MSYS2_DIR%\usr\bin;%PATH%"

  echo Updating pacman...
  bash -lc "pacman -S --needed --noconfirm pacman-mirrors"
  bash -lc "pacman -Syyu --noconfirm"

  echo Installing git...
  bash -lc "pacman -S --needed --noconfirm git"

  IF %BUILDTOOL%==autoconf (
    echo Installing autoconf tools...
    bash -lc "pacman -S --needed --noconfirm autoconf automake make"
  )

  IF %BUILDTOOL%==meson (
    echo Installing meson build tool...
    bash -lc "pacman -S --needed --noconfirm mingw-w64-%MSYS2_ARCH%-ninja mingw-w64-%MSYS2_ARCH%-meson"
  )

  IF %BUILDTOOL%==cmake (
    echo Installing cmake build tool...
    bash -lc "pacman -S --needed --noconfirm make mingw-w64-%MSYS2_ARCH%-cmake mingw-w64-%MSYS2_ARCH%-extra-cmake-modules"
  )

  echo Installing build and compile time dependencies...

  IF %TARGET%==importer (
    bash -lc "pacman -S --needed --noconfirm mingw-w64-%MSYS2_ARCH%-toolchain mingw-w64-%MSYS2_ARCH%-libtool mingw-w64-%MSYS2_ARCH%-libiconv mingw-w64-%MSYS2_ARCH%-libxml2 zip"

    cinst wget -x86

    wget https://github.com/rinigus/marisa-trie/archive/0.2.4.zip -O marisa.zip
    7z x marisa.zip
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && cd marisa-trie-0.2.4 && ./configure --disable-shared && make -j2 && make install"

    wget https://github.com/google/protobuf/releases/download/v3.1.0/protobuf-cpp-3.1.0.zip -O protobuf-cpp-3.1.0.zip
    7z x protobuf-cpp-3.1.0.zip
    bash -lc "cd ${APPVEYOR_BUILD_FOLDER} && cd protobuf-3.1.0 && ./configure --disable-shared && make -j2 && make install"

  ) ELSE (
    echo Installing common dependencies
    bash -lc "pacman -S --needed --noconfirm mingw-w64-%MSYS2_ARCH%-toolchain mingw-w64-%MSYS2_ARCH%-libtool mingw-w64-%MSYS2_ARCH%-libiconv mingw-w64-%MSYS2_ARCH%-protobuf mingw-w64-%MSYS2_ARCH%-libxml2 mingw-w64-%MSYS2_ARCH%-cairo mingw-w64-%MSYS2_ARCH%-pango mingw-w64-%MSYS2_ARCH%-qt5 mingw-w64-%MSYS2_ARCH%-glew mingw-w64-%MSYS2_ARCH%-glfw mingw-w64-%MSYS2_ARCH%-glm"
  )

  echo Finished installing MSYS2 build preconditions
)

IF %COMPILER%==msvc2015 (
  @echo on
  echo MSVC2015 build...

  echo Installing wget...
  cinst wget -x86

  IF %PLATFORM%==x64 (
    echo Downloading library dependencies...
    wget http://xmlsoft.org/sources/win32/64bit/zlib-1.2.8-win32-x86_64.7z -O zlib-1.2.8-win32-x86_64.7z
    wget http://xmlsoft.org/sources/win32/64bit/iconv-1.14-win32-x86_64.7z -O iconv-1.14-win32-x86_64.7z
    wget http://xmlsoft.org/sources/win32/64bit/libxml2-2.9.3-win32-x86_64.7z -O libxml2-2.9.3-win32-x86_64.7z

    echo Unpacking library dependencies...
    7z x zlib-1.2.8-win32-x86_64.7z -ozlib -y > nul
    7z x iconv-1.14-win32-x86_64.7z -oiconv -y > nul
    7z x libxml2-2.9.3-win32-x86_64.7z -olibxml2 -y > nul
    echo ...done
  )

  IF %BUILDTOOL%==meson (
    echo Installing meson build tool...
    set "PATH=C:\Python36-x64;C:\Python36-x64\Scripts;%PATH%"
    pip.exe install meson
    echo ...done
  )
)
