FROM debian:jessie

RUN apt-get update && apt-get install -y \
          git make libtool pkg-config \
          libxml2-dev libprotobuf-dev protobuf-compiler \
          libagg-dev \
          libfreetype6-dev \
          libcairo2-dev \
          libpangocairo-1.0-0 libpango1.0-dev \
          qt5-default qtdeclarative5-dev libqt5svg5-dev \
          freeglut3 freeglut3-dev \
          libmarisa-dev \
          locales \
    && rm -rf /var/lib/apt/lists/*

RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen
ENV LANG en_US.utf8

RUN apt-get update && apt-get install -y \
          autoconf automake \
          gcc \
    && rm -rf /var/lib/apt/lists/*
          
RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
