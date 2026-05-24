FROM ubuntu:24.04

# disable interactive functions
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -y \
    locales git make cmake pkg-config protobuf-compiler \
    g++-aarch64-linux-gnu qemu-user gpg \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.utf8

ENV SYSROOT=/usr/aarch64-linux-gnu/
ENV QEMU_LD_PREFIX=/usr/aarch64-linux-gnu/
ENV CXX=aarch64-linux-gnu-g++
ENV CC=aarch64-linux-gnu-gcc

RUN mkdir -p \
  $SYSROOT/etc/apt/sources.list.d \
  $SYSROOT/etc/apt/preferences.d \
  $SYSROOT/usr/share/dpkg \
  $SYSROOT/usr/lib \
  $SYSROOT/usr/bin \
  $SYSROOT/etc \
  $SYSROOT/var/cache \
  $SYSROOT/var/log/apt \
  $SYSROOT/var/lib/dpkg \
  $SYSROOT/var/lib/apt/lists/partial \
  $SYSROOT/var/lib/dpkg/updates \
  $SYSROOT/var/lib/dpkg/triggers \
  $SYSROOT/var/cache/apt/archives/partial

RUN touch \
  $SYSROOT/var/lib/dpkg/status \
  $SYSROOT/var/lib/dpkg/triggers/Unincorp \
  $SYSROOT/var/lib/dpkg/triggers/File

RUN echo arm64 > $SYSROOT/var/lib/dpkg/arch
RUN echo 'APT::Architecture "arm64";' > $SYSROOT/etc/apt/apt.conf
COPY arm64.list $SYSROOT/etc/apt/sources.list

RUN cp \
  /usr/share/dpkg/cputable \
  /usr/share/dpkg/tupletable \
  $SYSROOT/usr/share/dpkg/

ENV APTARGS="-o Dir=$SYSROOT -o Debug::NoLocking=1 -o Acquire::AllowInsecureRepositories=1 --allow-unauthenticated"

RUN mkdir -p $SYSROOT/etc/apt/trusted.gpg.d/
RUN gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 871920D1991BC93C
RUN gpg --export 871920D1991BC93C | tee $SYSROOT/etc/apt/trusted.gpg.d/ports.ubuntu.com.gpg

RUN apt-get \
  $APTARGS \
  update

RUN cd $SYSROOT/var/cache/apt/archives/ \
  && apt-get \
    $APTARGS \
    download -y \
    libprotobuf-dev:arm64 libprotobuf32t64:arm64 zlib1g:arm64 zlib1g-dev:arm64 catch2:arm64 \
  && find $SYSROOT/var/cache/apt/archives/ -name '*.deb' -exec /usr/bin/dpkg -x {} $SYSROOT \;

RUN mkdir /work
COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
