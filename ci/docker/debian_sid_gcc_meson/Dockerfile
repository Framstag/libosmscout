FROM debian:sid

RUN apt-get update && apt-get install -y \
          git make libtool pkg-config \
          libxml2-dev libprotobuf-dev protobuf-compiler \
          libagg-dev \
          libfreetype6-dev \
          libcairo2-dev \
          libpangocairo-1.0-0 libpango1.0-dev \
          qt5-default qtdeclarative5-dev libqt5svg5-dev qtlocation5-dev qtpositioning5-dev qttools5-dev-tools \
          freeglut3 freeglut3-dev \
          libmarisa-dev \
          doxygen \
          swig openjdk-8-jdk \
          locales \
    && rm -rf /var/lib/apt/lists/*

RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen
ENV LANG en_US.utf8

ENV JAVA_HOME /usr/lib/jvm/java-8-openjdk-amd64/

RUN apt-get update && apt-get install -y \
          gcc \
          meson \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
