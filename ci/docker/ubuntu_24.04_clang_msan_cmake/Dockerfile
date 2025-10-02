FROM ubuntu:noble

# disable interactive functions
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -y \
          git make libtool pkg-config \
          doxygen \
          locales \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG en_US.utf8
ENV JAVA_HOME /usr/lib/jvm/java-8-openjdk-amd64/
ENV QT_QPA_PLATFORM offscreen

RUN apt-get update && apt-get install -y \
          cmake \
          clang-20 libclang-20-dev \
    && rm -rf /var/lib/apt/lists/*
          
RUN mkdir /work

# Build libc++ with memory instrumentation
RUN git clone --depth=1 https://github.com/llvm/llvm-project && \
    cd llvm-project && \
    mkdir build && cd build && \
    cmake -S ../runtimes \
        -DCMAKE_BUILD_TYPE=RelWithDebug \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
        -DLIBCXXABI_USE_LLVM_UNWINDER=off \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins \
        -DCMAKE_C_COMPILER=/usr/bin/clang-20 \
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++-20 && \
      make -j $(nproc) && \
      make install && \
      ldconfig -v && \
      cd ../.. && rm -rf llvm-project

# Build Catch2 with memory instrumentation
RUN git clone --depth=1 https://github.com/catchorg/Catch2.git && \
    cd Catch2 && \
    mkdir build && cd build && \
    cmake -S .. \
      -DCMAKE_BUILD_TYPE=RelWithDebug \
      -DCMAKE_C_COMPILER=/usr/bin/clang-20 \
      -DCMAKE_CXX_COMPILER=/usr/bin/clang++-20 \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fsanitize=memory" \
      -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -fsanitize=memory" \
      -DCMAKE_UNITY_BUILD=ON -Wno-dev && \
    make -j $(nproc) && \
    make install && \
    ldconfig -v && \
    cd ../.. && rm -rf Catch2

COPY data/build.sh /work
RUN chmod +x /work/build.sh

WORKDIR /work
CMD ./build.sh
