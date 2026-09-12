# syntax=docker/dockerfile:1
#
# mapgen image: regeneration script + pinned Import tool.
#
# Build:
#   docker build -f scripts/mapgen/Dockerfile -t osmscout-mapgen:<version> .
#
# Run (one pass per start, external scheduler restarts it):
#   docker run --rm --read-only \
#     -v /var/lib/osmscout-mapgen:/work \
#     -v /repository:/repository \
#     -v /etc/osm-maps:/config:ro \
#     osmscout-mapgen:<version>
#
# Mounts:
#   /work          transient: downloads, import intermediate data
#   /repository  database repository (served tree + admin/ + staging/)
#   /config        read-only: imports.json + names.json

# ---- build stage: minimized cmake Release build of core + import + Import tool
FROM ubuntu:noble AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake g++ make pkg-config \
    liblzma-dev libmarisa-dev libprotobuf-dev \
    libtbb-dev libxml2-dev protobuf-compiler zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DOSMSCOUT_BUILD_IMPORT=ON \
    -DOSMSCOUT_BUILD_TOOL_IMPORT=ON \
    -DOSMSCOUT_BUILD_GPX=OFF \
    -DOSMSCOUT_BUILD_MAP_AGG=OFF \
    -DOSMSCOUT_BUILD_MAP_CAIRO=OFF \
    -DOSMSCOUT_BUILD_MAP_OPENGL=OFF \
    -DOSMSCOUT_BUILD_MAP_QT=OFF \
    -DOSMSCOUT_BUILD_MAP_SVG=OFF \
    -DOSMSCOUT_BUILD_MAP_SKIA=OFF \
    -DOSMSCOUT_BUILD_CLIENT=OFF \
    -DOSMSCOUT_BUILD_DEMOS=OFF \
    -DOSMSCOUT_BUILD_TOOL_DUMPDATA=OFF \
    -DOSMSCOUT_BUILD_TOOL_MCPSERVER=OFF \
    -DOSMSCOUT_BUILD_TOOL_PUBLICTRANSPORTMAP=OFF \
    -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF \
    -DOSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF \
    -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF \
    -DOSMSCOUT_BUILD_DOC_API=OFF \
    -DOSMSCOUT_BUILD_WEBPAGE=OFF \
    && cmake --build build --target Import -j"$(nproc)"

# ---- runtime stage: Import binary + dependencies + script, non-root, read-only rootfs
FROM ubuntu:noble AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl jq \
    libgomp1 liblzma5 libmarisa0 libprotobuf32 \
    libtbb12 libxml2 libzstd1 zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/Import/Import /usr/local/bin/Import
COPY --from=build /src/build/libosmscout/libosmscout.so.1.1.1 /usr/local/lib/
COPY --from=build /src/build/libosmscout-import/libosmscout_import.so.1.1.1 /usr/local/lib/

RUN ln -s libosmscout.so.1.1.1 /usr/local/lib/libosmscout.so.1 \
 && ln -s libosmscout.so.1.1.1 /usr/local/lib/libosmscout.so \
 && ln -s libosmscout_import.so.1.1.1 /usr/local/lib/libosmscout_import.so.1 \
 && ln -s libosmscout_import.so.1.1.1 /usr/local/lib/libosmscout_import.so \
 && ldconfig \
 && ldd /usr/local/bin/Import | awk '/not found/{print "missing:", $0; exit 1}' \
 && ldd /usr/local/bin/Import | grep "=> /" | awk '{print $3}' | sort -u | wc -l

COPY stylesheets/map.ost /usr/local/share/osmscout/map.ost
COPY scripts/mapgen/mapgen.sh /usr/local/bin/mapgen.sh

RUN chmod +x /usr/local/bin/mapgen.sh \
 && useradd --system --uid 1000 --home-dir /work -o mapgen

ENV MAPGEN_TYPEFILE=/usr/local/share/osmscout/map.ost

USER mapgen

ENTRYPOINT ["/usr/local/bin/mapgen.sh"]
