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

# Stage the built libraries together with their symlink chain, so the runtime
# stage copies them without naming a version anywhere: the library version
# lives in the build system alone (CMakeLists.txt, meson.build).
RUN mkdir -p /stage/lib \
    && cp -a build/libosmscout/libosmscout.so* build/libosmscout-import/libosmscout_import.so* /stage/lib/

# ---- runtime stage: Import binary + dependencies + script, non-root, read-only rootfs
FROM ubuntu:noble AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl jq \
    libgomp1 liblzma5 libmarisa0 libprotobuf32 \
    libtbb12 libxml2 libzstd1 zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/Import/Import /usr/local/bin/Import
COPY --from=build /stage/lib/ /usr/local/lib/

RUN ldconfig \
 && ldd /usr/local/bin/Import | awk '/not found/{print "missing:", $0; exit 1}'

COPY stylesheets/map.ost /usr/local/share/osmscout/map.ost
COPY scripts/mapgen/mapgen.sh /usr/local/bin/mapgen.sh

RUN chmod +x /usr/local/bin/mapgen.sh \
 && (getent group 1000 >/dev/null 2>&1 || groupadd --gid 1000 mapgen) \
 && useradd --system --uid 1000 --gid 1000 --home-dir /work -o mapgen

ENV MAPGEN_TYPEFILE=/usr/local/share/osmscout/map.ost
# HOME points at the writable mount: a foreign uid has no entry in /etc/passwd,
# so tools that look for a home directory would otherwise find none.
ENV HOME=/work

USER mapgen

ENTRYPOINT ["/usr/local/bin/mapgen.sh"]

# OCI metadata. The publishing pass of the image workflow overrides these with
# the values of the revision it publishes (see .github/workflows/mapgen_image.yml).
ARG OSMSCOUT_SOURCE="https://github.com/Framstag/libosmscout"
ARG OSMSCOUT_REVISION="unknown"
ARG OSMSCOUT_VERSION="unknown"
ARG OSMSCOUT_CREATED="unknown"

LABEL org.opencontainers.image.title="libosmscout mapgen" \
      org.opencontainers.image.description="Regenerates OSM map databases into a map repository" \
      org.opencontainers.image.source="${OSMSCOUT_SOURCE}" \
      org.opencontainers.image.revision="${OSMSCOUT_REVISION}" \
      org.opencontainers.image.version="${OSMSCOUT_VERSION}" \
      org.opencontainers.image.created="${OSMSCOUT_CREATED}" \
      org.opencontainers.image.licenses="LGPL-2.1-or-later"
