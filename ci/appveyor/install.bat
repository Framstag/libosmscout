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

  IF %BUILDTOOL%==cmake (
    echo Downloading vcpkg...

    cd c:\tools\vcpkg

    echo Installing zlib...
    .\vcpkg install zlib:x64-windows
    echo ...done

    echo Installing libiconv...
    .\vcpkg install libiconv:x64-windows
    echo ...done

    echo Installing libxml2...
    .\vcpkg install libxml2:x64-windows
    echo ...done

    echo Installing protobuf...
    .\vcpkg install protobuf:x64-windows
    echo ...done

    echo Installing cairo...
    .\vcpkg install cairo:x64-windows
    echo ...done

    echo Installing pango...
    .\vcpkg install pango:x64-windows
    echo ...done

    echo Installing qt5-base...
    .\vcpkg install qt5-base:x64-windows
    echo ...done

    echo Installing qt5-declarative...
    .\vcpkg install qt5-declarative:x64-windows
    echo ...done
	
    echo Installing qt5-svg...
    .\vcpkg install qt5-svg:x64-windows
    echo ...done

    echo Installing qt5-tools...
    .\vcpkg install qt5-tools:x64-windows
    echo ...done
	
    echo Installing OpenGL...
    .\vcpkg install opengl:x64-windows
    echo ...done

    echo Installing freeglut...
    .\vcpkg install freeglut:x64-windows
    echo ...done

    echo Installing glm...
    .\vcpkg install glm:x64-windows
    echo ...done

    echo Installing glew...
    .\vcpkg install glew:x64-windows
    echo ...done

    echo Installing glfw3...
    .\vcpkg install glfw3:x64-windows
    echo ...done
	
    echo System-wide integrating vcpkg...
    .\vcpkg integrate install
    echo ...done
  
    cd %APPVEYOR_BUILD_FOLDER%
  )	
  
  IF %BUILDTOOL%==meson (
    echo Installing meson build tool...
    set "PATH=C:\Python36-x64;C:\Python36-x64\Scripts;%PATH%"
    pip.exe install meson
    echo ...done
  )
)
