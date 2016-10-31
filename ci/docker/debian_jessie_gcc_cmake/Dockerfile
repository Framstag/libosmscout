FROM debian:jessie

RUN apt-get update && apt-get install -y \
          git make libtool pkg-config \
          libxml2-dev libprotobuf-dev protobuf-compiler \
          libagg-dev \
          libfreetype6-dev \
          libcairo2-dev \
          libpangocairo-1.0-0 libpango1.0-dev \
          qt5-default qtdeclarative5-dev libqt5svg5-dev qtpositioning5-dev \
          freeglut3 freeglut3-dev \
          libmarisa-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
          cmake \
          gcc \
    && rm -rf /var/lib/apt/lists/*
          
RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
