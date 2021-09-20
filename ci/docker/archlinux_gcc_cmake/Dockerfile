FROM archlinux:base

RUN pacman --noconfirm --needed -Sy \
          git \
          libxml2 protobuf \
          freetype2 \
          cairo pango \
          qt5-base qt5-declarative qt5-graphicaleffects qt5-location \
          qt5-quickcontrols qt5-script qt5-sensors qt5-svg qt5-tools \
          qt5-translations qt5-multimedia \
          freeglut glu glew glm glfw libxcursor libxinerama \
          marisa

RUN pacman --noconfirm --needed -S \
          cmake make pkg-config \
          gcc

RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
