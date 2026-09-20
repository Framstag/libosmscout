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
# Run (pass on a schedule, container stays up):
#   docker run -d --read-only -e MAPGEN_CRON='0 */6 * * *' \
#     -v /var/lib/osmscout-mapgen:/work \
#     -v /repository:/repository \
#     -v /etc/osm-maps:/config:ro \
#     osmscout-mapgen:<version>
#
# Arguments run the pass once with those arguments whatever else is set, which
# is how "--check-config" and an explicit single run work.
#
# Mounts:
#   /work          transient: downloads, import intermediate data
#   /repository  database repository (served tree + admin/ + staging/)
#   /config        read-only: imports.json + names.json + basemap.json
#                  (names.json is read from the served root, see below)
#
# Environment of interest to a deployment:
#   MAPGEN_TYPEFILE            regional type definitions (default: map.ost)
#   MAPGEN_BASEMAP_TYPEFILE    basemap type definitions (default: basemap.ost)
#   MAPGEN_COASTLINES_URL      default coastline source of the basemap; a
#                              basemap.json may override it

# ---- build stage: minimized cmake Release build of core + import + Import tool
FROM ubuntu:noble AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake g++ make pkg-config \
    liblzma-dev libmarisa-dev libprotobuf-dev \
    libtbb-dev libxml2-dev nlohmann-json3-dev protobuf-compiler zlib1g-dev \
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
    && cmake --build build --target Import BasemapImport -j"$(nproc)"

# Stage the built libraries together with their symlink chain, so the runtime
# stage copies them without naming a version anywhere: the library version
# lives in the build system alone (CMakeLists.txt, meson.build).
RUN mkdir -p /stage/lib \
    && cp -a build/libosmscout/libosmscout.so* build/libosmscout-import/libosmscout_import.so* /stage/lib/

# ---- runtime stage: Import binary + dependencies + script, non-root, read-only rootfs
FROM ubuntu:noble AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl jq unzip util-linux \
    libgomp1 liblzma5 libmarisa0 libprotobuf32 \
    libtbb12 libxml2 libzstd1 zlib1g \
    && rm -rf /var/lib/apt/lists/*

# The scheduler of the scheduled mode (see scripts/mapgen/mapgen-entrypoint.sh).
# A static binary, chosen by target architecture and verified against the
# checksum pinned for it: the distribution's cron daemon needs root and a
# writable /var/spool/cron, which this image does not allow.
ARG SUPERCRONIC_VERSION="v0.2.49"
ARG TARGETARCH
RUN case "${TARGETARCH:-amd64}" in \
      amd64) asset="supercronic-linux-amd64"; sum="a53ae236602c7338aba3fbaff40bda6300eae3b9fedb8261eb06cfe3724430c1" ;; \
      arm64) asset="supercronic-linux-arm64"; sum="02aa0cb229ba09050cba6638059dadb9eedc2276632ea43d6a57a2f8c1629dd5" ;; \
      *) echo "unsupported architecture for the scheduler: ${TARGETARCH}" >&2; exit 1 ;; \
    esac \
 && curl -fsSL -o /usr/local/bin/supercronic \
      "https://github.com/aptible/supercronic/releases/download/${SUPERCRONIC_VERSION}/${asset}" \
 && echo "${sum}  /usr/local/bin/supercronic" | sha256sum -c - \
 && chmod +x /usr/local/bin/supercronic

COPY --from=build /src/build/Import/Import /usr/local/bin/Import
COPY --from=build /src/build/BasemapImport/BasemapImport /usr/local/bin/BasemapImport
COPY --from=build /stage/lib/ /usr/local/lib/

RUN ldconfig \
 && ldd /usr/local/bin/Import | awk '/not found/{print "missing:", $0; exit 1}' \
 && ldd /usr/local/bin/BasemapImport | awk '/not found/{print "missing:", $0; exit 1}'

# The whole type definition set: the type file pulls in modules that are looked
# up beside it, so shipping the file alone makes every import fail with
# "Cannot load type configuration!".
COPY stylesheets/*.ost /usr/local/share/osmscout/
COPY scripts/mapgen/mapgen.sh /usr/local/bin/mapgen.sh
COPY scripts/mapgen/mapgen-basemap.sh /usr/local/bin/mapgen-basemap.sh
COPY scripts/mapgen/mapgen-pass.sh /usr/local/bin/mapgen-pass.sh
COPY scripts/mapgen/mapgen-entrypoint.sh /usr/local/bin/mapgen-entrypoint.sh

RUN chmod +x /usr/local/bin/mapgen-basemap.sh /usr/local/bin/mapgen-pass.sh /usr/local/bin/mapgen-entrypoint.sh

RUN chmod +x /usr/local/bin/mapgen.sh \
 && (getent group 1000 >/dev/null 2>&1 || groupadd --gid 1000 mapgen) \
 && useradd --system --uid 1000 --gid 1000 --home-dir /work -o mapgen

ENV MAPGEN_TYPEFILE=/usr/local/share/osmscout/map.ost
# The basemap type file is bundled as well; it includes no modules, so the file
# itself is the complete set.
ENV MAPGEN_BASEMAP_TYPEFILE=/usr/local/share/osmscout/basemap.ost
# Default coastline source of the basemap. The upstream dataset is derived from
# OpenStreetMap coastline data (ODbL) and is refreshed about daily; the pass
# checks it with a conditional request and adopts a newer copy on the interval
# configured in basemap.json. Overridable per deployment.
ARG OSMSCOUT_COASTLINES_URL="https://osmdata.openstreetmap.de/download/coastlines-split-4326.zip"
ENV MAPGEN_COASTLINES_URL=${OSMSCOUT_COASTLINES_URL}
# HOME points at the writable mount: a foreign uid has no entry in /etc/passwd,
# so tools that look for a home directory would otherwise find none.
ENV HOME=/work

USER mapgen

ENTRYPOINT ["/usr/local/bin/mapgen-entrypoint.sh"]

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
