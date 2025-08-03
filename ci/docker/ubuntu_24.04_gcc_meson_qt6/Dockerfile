FROM ubuntu:noble

# disable interactive functions
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -y \
          git make libtool pkg-config \
          qmake6 libqt6svg6-dev libqt6core5compat6-dev qt6-declarative-dev qt6-positioning-dev qt6-tools-dev-tools \
          qt6-l10n-tools qt6-tools-dev qt6-multimedia-dev \
          libgl-dev \
          locales \
          meson ninja-build g++ \
          catch2 \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG en_US.utf8
ENV QT_QPA_PLATFORM offscreen

# RUN apt-get update && apt-get install -y \
#           g++ \
#           python3-pip \
#     && rm -rf /var/lib/apt/lists/*
#
# RUN pip install meson ninja

RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
