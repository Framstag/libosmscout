FROM pritunl/archlinux:latest

RUN pacman --noconfirm --needed -S \
          git \
          libxml2 protobuf \
          agg freetype2 \
          cairo \
          qt5-base qt5-declarative qt5-graphicaleffects qt5-location \
          qt5-quickcontrols qt5-script qt5-sensors  qt5-svg qt5-tools \
          freeglut glu \
          marisa

RUN pacman --noconfirm --needed -S \
          make autoconf automake pkg-config \
          clang
          
RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
