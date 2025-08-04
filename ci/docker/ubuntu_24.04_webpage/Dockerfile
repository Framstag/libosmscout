FROM ubuntu:noble

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -y \
      wget git python3-pygments doxygen lftp \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/gohugoio/hugo/releases/download/v0.147.8/hugo_0.147.8_linux-amd64.deb \
  && dpkg -i hugo_0.147.8_linux-amd64.deb

RUN mkdir /work

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
