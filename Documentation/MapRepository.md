# Map Repository — generation, metadata, and client update contract

This document describes the complete map repository pipeline: how maps are
generated from OSM extracts, what metadata is produced, how the repository
is laid out on the server, how the regeneration script and container work,
and how clients detect updates.

The pipeline is defined by the OpenSpec changes `map-meta-structure`
(`openspec/changes/map-meta-structure/`) for the regional databases and
`mapgen-basemap-generation` (`openspec/changes/mapgen-basemap-generation/`)
for the world basemap. The specs there are the normative contract; this
document is the operational guide.

```
+--------------------------------------------------------------+
|  OVERVIEW                                                     |
+--------------------------------------------------------------+
|                                                              |
|  imports.json (F1)   names.json (F2)   basemap.json (F4)  |
|  id -> download URL  id -> names        planet + coastline |
|        |                    |                 |          |
|        v                    v                 v          |
|  +-------------------------------------------------------+ |
|  |  mapgen.sh (regeneration script)                      | |
|  |  curl -> md5 verify -> Import ->                      | |
|  |  place -> prune -> records                            | |
|  |  mapgen-basemap.sh: Import + water index -> slot      | |
|  +-------------------------------------------------------+ |
|        |                                                   |
|        v                                                   |
|  /repository (repository volume)                          |
|    public/  names.json, <region-index-path>/v<version>/*    |
|             basemap/index.json, basemap/v<version>/*        |
|    private/ admin/ (records), staging/ (atomic swap)        |
|        |                                                   |
|        v                                                   |
|  client: GET names.json -> probe own version -> compare     |
|          generatedAt -> download -> verify crc32            |
|          GET basemap/index.json -> compare local metadata   |
+--------------------------------------------------------------+
```

## 1. The configuration and metadata files

All files are JSON with a `schema` version field. Consumers refuse to
process unknown schema versions.

### 1.1 imports.json (F1) — the imports manifest

Lists every supported import: a unique id, the download URL, and optional
per-id overrides for refresh frequency and retention depth. Global defaults
apply when an id does not override them.

```json
{
  "schema": 1,
  "history": 3,
  "refresh": 7,
  "imports": [
    {
      "id": "berlin",
      "url": "https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf"
    },
    {
      "id": "dortmund",
      "url": "https://download.geofabrik.de/europe/germany/nordrhein-westfalen/dortmund-latest.osm.pbf",
      "history": 5,
      "refresh": 1
    }
  ]
}
```

| Field     | Meaning                                                          |
|-----------|------------------------------------------------------------------|
| `schema`  | Format version; consumers reject unknown values                  |
| `history` | Global default: how many type-config versions of a database the server keeps per import |
| `refresh` | Global default: days between change checks per import           |
| `id`      | Unique import id; must appear as a leaf in names.json            |
| `url`     | Download URL of the source extract (Geofabrik or any HTTP source) |
| `history` | Per-id override of the retention depth                            |
| `refresh` | Per-id override of the check frequency                            |

The download service is expected to publish an md5 sidecar at `<url>.md5`
in `md5sum` output format (`<hash>  <filename>`). Geofabrik publishes these
for every extract.

### 1.2 names.json (F2) — the region index

Defines the hierarchical structure and localized display names. Internal
nodes have `children`; leaf nodes reference import ids from F1. The tree is
used by the client UI for display and by the script as the layout hint for
server placement.

```json
{
  "schema": 1,
  "regions": [
    {
      "id": "europe",
      "names": {"en": "Europe", "de": "Europa", "fr": "Europe"},
      "children": [
        {
          "id": "germany",
          "names": {"en": "Germany", "de": "Deutschland"},
          "children": [
            {"id": "berlin", "names": {"en": "Berlin", "de": "Berlin"}}
          ]
        }
      ]
    }
  ]
}
```

Consistency rules (enforced by the script's config validation):

- every leaf id must exist in imports.json,
- every import id must be referenced by exactly one leaf,
- ids are append-only: renaming or moving a leaf changes the server
  placement path and must be treated as a migration, not a casual edit.

### 1.3 db.json (F3) — per-database metadata

Written by the Import tool into the destination directory at the end of a
successful import run. It is copied verbatim to the server and never
mutated afterwards. It contains everything a client or the script needs to
verify and manage the database.

```json
{
  "schema": 1,
  "typeConfigVersion": 27,
  "generatedAt": "2026-09-07T16:35:44Z",
  "source": {
    "url": "https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf",
    "md5": "146f59bf3b42630f89572160de6260bb"
  },
  "import": {
    "tool": "Import",
    "version": "1.1.1",
    "startStep": 1,
    "endStep": 28,
    "durationSeconds": 0.6
  },
  "output": {
    "boundingBox": {
      "minLon": 13.4, "minLat": 52.5, "maxLon": 13.43, "maxLat": 52.53
    },
    "files": {
      "map.lib": {"size": 12345678, "crc32": 2912136757},
      "nodes.dat": {"size": 987654321, "crc32": 316299821}
    }
  },
  "stats": {"types": 671}
}
```

| Field                | Meaning                                                        |
|----------------------|----------------------------------------------------------------|
| `schema`             | Format version                                                  |
| `typeConfigVersion`  | `FILE_FORMAT_VERSION` of the database (the type-config version) |
| `generatedAt`        | ISO 8601 UTC creation timestamp                                 |
| `source.url`         | Download URL, passed to the import tool via `--source-url`      |
| `source.md5`         | Verified source hash, passed via `--source-md5`                 |
| `import.tool/version`| Import tool identity and version                                |
| `import.startStep/endStep` | Executed import pipeline steps                            |
| `import.durationSeconds` | Run duration                                               |
| `output.boundingBox` | Bounding box of the imported data (from `bounding.dat`)         |
| `output.files`       | Inventory: relative file name -> byte size + CRC-32             |
| `stats.types`        | Number of defined types                                         |

Properties:

- **Atomic write**: db.json is written to `db.json.tmp` and renamed, so a
  reader never observes a partially written file.
- **Only on success**: a failed import writes no db.json. Presence of
  db.json is the completeness marker for the whole database.
- **Relative references**: all file names are relative; the file can be
  copied verbatim together with the database.
- **Locale-independent**: numbers use `.` decimal separator and no
  thousands grouping regardless of the process locale.

### 1.4 basemap.json (F4) — the basemap configuration

Describes the world basemap: where its inputs come from and how often it may
be checked and rebuilt. It is a separate file because the basemap is not a
region: it has no entry in imports.json and none in names.json, and the
region index must keep matching the imports manifest exactly.

```json
{
  "schema": 1,
  "refresh": 7,
  "coastlinesRefresh": 90,
  "history": 2,
  "extract": "/config/planet_extract.osm.pbf",
  "coastlines": {
    "url": "https://osmdata.openstreetmap.de/download/coastlines-split-4326.zip"
  },
  "importOptions": {
    "waterIndexMinMag": 6,
    "waterIndexMaxMag": 6,
    "lowZoomOptMaxMag": 6,
    "areaNodeGridMag": 6,
    "langOrder": "en,#",
    "minIndexLevel": 4,
    "maxIndexLevel": 10,
    "maxWaterDistance": 2048
  }
}
```

| Field                | Meaning                                                        |
|----------------------|----------------------------------------------------------------|
| `schema`             | Format version                                                  |
| `refresh`            | Days between checks of the basemap inputs                       |
| `coastlinesRefresh`  | Days between adoptions of a newer coastline copy; must not be shorter than `refresh` |
| `history`            | How many basemap versions the server keeps; `0` keeps all        |
| `extract`            | Pre-filtered planet export, read from the configuration area    |
| `coastlines.url`     | Coastline source; the image's default applies when absent        |
| `coastlines.sha256`  | Optional checksum of the coastline archive, enforced when present |
| `importOptions`      | Optional tuning passed to the import tools; unknown keys are rejected |

The file is required: a pass without it stops before doing any work, because a
missing file is easier to notice than a basemap that is silently absent. See
`scripts/mapgen/basemap.example.json` for the annotated version.

The planet export is an OSM extract filtered to what `stylesheets/basemap.ost`
defines (coastlines, country boundaries, continents, cities, oceans, countries,
seas); `basemap.ost` documents the filter. It is the input whose content
decides whether a new basemap is built.

## 2. Server directory layout

```
/repository/                       repository volume
  public/                            served root (webserver)
    names.json                       region index (F2)
    europe/
      germany/
        berlin/
          v27/                       one slot per type-config version
            db.json                  database metadata (F3)
            map.lib, *.dat, ...      database files
          v26/  ...                  older slots, kept per retention
    basemap/
      index.json                     availability manifest (versions + change times)
      v27/                           the world basemap, same shape as a regional slot
        db.json                      metadata, including the water index
        types.dat, *.dat, water.idx  database files
      v26/  ...                      older slots, kept per retention
  private/                           never served (outside served root)
    admin/
      berlin_v27_generation.json     per-slot run records
      berlin_check.json              check-cycle state
      basemap_check.json             basemap input state (extract, coastline, adoption)
      basemap_v27_generation.json    basemap placement records
    staging/
      berlin/v27.new/                staging for atomic replacement
      basemap/v27.new/               staging for the basemap slot
```

- The served root is `<repo>/public/`; point the webserver there. The
  private part needs no rewrite or deny rule because it is structurally
  outside the served root (see the example nginx config in
  `scripts/mapgen/nginx.example.conf`).
- The region index `names.json` lives at the served root; the script reads
  it from there (`MAPGEN_NAMES_FILE`), so the served copy is the single
  source of truth for layout and display.

- The leaf path in names.json gives the placement position below the
  served root; the `typeConfigVersion` from db.json keys the slot
  directory.
- There is at most **one database per (import, type-config version)**.
  A new import of the same version replaces the slot.
- The basemap is laid out like a regional database - `basemap/v<version>/`
  with a db.json and the data files - plus `basemap/index.json`, the
  availability manifest a client reads to learn which versions the server
  offers. It is a database directory like any other, so the client downloads
  it file by file and verifies it the same way.
- The client-accessible surface is exactly: names.json, per-database
  db.json, database files, and the basemap manifest.

## 3. Integrity model

Two layers, each fit for its purpose:

| Layer   | What it protects            | Mechanism                                        |
|---------|-----------------------------|--------------------------------------------------|
| Source  | authenticity of the extract | md5 sidecar published by the download service, verified by the script with `md5sum` |
| Basemap coastline | a corrupt or truncated coastline archive | the unpack utility's CRC-32 per archive entry, plus an optional checksum configured per deployment (the upstream source publishes none) |
| Outputs | torn copies, disk rot, buggy transfers | CRC-32 per file in db.json, computed by the Import tool |

The CRC-32 is a self-implemented table-based IEEE 802.3 CRC-32
(`libosmscout/include/osmscout/io/Crc32.h`, `libosmscout/src/osmscout/io/Crc32.cpp`),
bit-identical to zlib's `crc32()` — verified against the standard check
value and cross-checked against zlib on real import output. It lives in
the libosmscout library (public API `osmscout::Crc32` and
`osmscout::ComputeFileCrc32`), so clients can verify with the library
itself or any standard CRC-32 implementation.

**Do not use POSIX `cksum`** to verify the emitted crc32 values — it uses a
different CRC parametrization. Script-side verification uses
`python3 -c "import zlib; ..."` or is skipped in favor of client-side
verification.

## 4. The regeneration script

`scripts/mapgen/mapgen.sh` performs one complete pass over the imports
manifest and exits. Run it under an external scheduler (cron, systemd
timer, kubernetes CronJob); the script itself never schedules.

### Environment

| Variable               | Default          | Meaning                                  |
|------------------------|------------------|------------------------------------------|
| `MAPGEN_CONFIG_DIR`    | `/config`        | Directory with imports.json             |
| `MAPGEN_REPO_DIR`      | `/repository`  | Repository volume (public/ + private/)    |
| `MAPGEN_PUBLIC_DIR`    | `<repo>/public`  | Served root: names.json + database slots |
| `MAPGEN_PRIVATE_DIR`   | `<repo>/private` | Script records (admin/) + staging/       |
| `MAPGEN_NAMES_FILE`    | `<public>/names.json` | Region index (served copy)            |
| `MAPGEN_WORK_DIR`      | `/work`          | Transient work area                     |
| `MAPGEN_IMPORT`        | `Import`         | Import tool binary                      |
| `MAPGEN_TYPEFILE`      | `map.ost`        | Type definition file; modules it names have to sit beside it |
| `MAPGEN_BASEMAP_FILE`  | `<config>/basemap.json` | Basemap configuration (F4)           |
| `MAPGEN_BASEMAP_SCRIPT`| next to `mapgen.sh` | The basemap step script               |
| `MAPGEN_BASEMAP_IMPORT`| `BasemapImport`  | Water index tool of the basemap         |
| `MAPGEN_BASEMAP_TYPEFILE` | `basemap.ost` | Basemap type definition file            |
| `MAPGEN_COASTLINES_URL`| image value      | Default coastline source of the basemap |
| `MAPGEN_UNZIP`         | `unzip`          | Unpack utility for the coastline archive |
| `MAPGEN_SCHEMA_VERSION`| `1`              | Supported schema version                |

### Flow per import

```
due?  now - lastCheckedAt >= refresh*86400
  no  -> skip (no network activity)
  yes ->
    fetch <url>.md5 -> published hash
    persist private/admin/<id>_check.json (lastCheckedAt, lastSeenSourceMd5)
    published hash == newest db.json source.md5?
      yes -> skip (source unchanged)
      no  -> download extract
             verify md5 against published hash
               mismatch -> fail, no import
             run Import --source-url <url> --source-md5 <hash>
               failure -> fail, no placement
             read typeConfigVersion from db.json
             stage private/staging/<id>/v<version>.new (inventory files + db.json)
             atomic swap into <repo>/public/<path>/v<version>
             prune oldest version slots beyond retention depth
             write private/admin/<id>_v<version>_generation.json
```

### The basemap step

After the regional loop, and inside the same pass and the same lock,
`mapgen.sh` runs `mapgen-basemap.sh`. A basemap that fails is reported as a
failure of the pass, but it neither undoes nor blocks the regional work that
was already done next to it.

```
refresh elapsed?  now - lastCheckedAt >= refresh*86400
  no  -> skip (no network activity, no hashing)
  yes ->
    hash the planet export
    conditional GET of the coastline archive (If-Modified-Since, work-area copy reused)
      verify the configured checksum when one is set
      unpack (the unpack utility validates the archive's own checksums)
    extract content unchanged and coastline unchanged         -> record check, done
    coastline changed but adoption not due
                                                             -> record check, keep serving
    otherwise ->
      Import --typefile basemap.ost <importOptions>           (output cached by extract hash)
      BasemapImport --coastlines <unpacked .shp> <options>    (this owns water.idx)
      check the produced file set a client requires
      read typeConfigVersion and generatedAt from db.json
      stage private/staging/basemap/v<version>.new
      atomic swap into <repo>/public/basemap/v<version>
      prune older basemap slots beyond history
      write public/basemap/index.json (versions + change times)
      write private/admin/basemap_v<version>_generation.json
      record the served input hashes and the adoption time
```

Two cadences, because the upstream coastline data is regenerated about daily:
`refresh` decides how often the inputs may be looked at, `coastlinesRefresh`
decides how often a newer coastline copy may actually be adopted. Checking
frequently therefore does not mean rebuilding frequently.

The import output is cached in the work area under the hash of the planet
export, so a pass that was triggered by new coastline data re-runs only the
water index step. Only the output of the export that is currently served is
kept: stale caches of earlier exports are removed after a successful placement,
so the work area does not grow with the number of exports. With the basemap
configured, the work area holds the coastline archives (the adopted one and the
newest fetched one), their unpacked shape file, and that one import output -
several gigabytes of transient data.

The water index of a basemap comes from the world coastline
data, not from the coastline ways of the export, so `BasemapImport` runs after
the import and replaces the index the import wrote.

### Script-owned state (private/admin/)

| File                          | Content                                        |
|-------------------------------|------------------------------------------------|
| `<id>_check.json`             | `lastCheckedAt` (epoch), `lastSeenSourceMd5`    |
| `<id>_v<version>_generation.json` | placement time, server path, retention, pruned versions, source change result, run outcome |
| `basemap_check.json`          | `lastCheckedAt`, `lastAdoptedAt`, `extractMd5`, `coastlinesMd5`, `coastlinesLastModified` |
| `basemap_v<version>_generation.json` | placement time, generation time, pruned versions |

The script derives its previous state from the server tree itself (the
newest db.json's `source.md5`), so the admin records are documentary, not
functional prerequisites.

### Atomic replacement

The slot is replaced by renaming directories on the same filesystem:

```
mv <slot> private/staging/<id>/v<version>.old
mv private/staging/<id>/v<version>.new <slot>
rm -rf private/staging/<id>/v<version>.old
```

Readers see either the complete old database or the complete new database,
never a mixture. Staging lives inside the repository volume next to the
served root (`private/` beside `public/`), so the renames stay on one
filesystem (a cross-filesystem rename would fail with EXDEV).

### Failure handling

- Failures are reported per import; the script exits non-zero if any
  import failed.
- A failed import never places a database and never leaves partial state.
- A missing db.json (e.g. after rollback to an older Import binary) is
  treated as "source changed": the next run imports once and recreates it.
- A failing import stays due, so the next run retries it: the check state is
  recorded when the source was found unchanged or the database was placed,
  never for an attempt. Without that, a single failure would silently wait out
  the whole `refresh` window.
- A download is retried a few times within the same run, resumes an interrupted
  transfer instead of starting over, abandons a transfer that stops delivering
  data, and discards a download that fails verification rather than keeping it.
  The tunables are `MAPGEN_CONNECT_TIMEOUT`, `MAPGEN_HASH_TIMEOUT`,
  `MAPGEN_IDLE_TIMEOUT`, `MAPGEN_IDLE_SPEED_LIMIT`, `MAPGEN_DOWNLOAD_ATTEMPTS`
  and `MAPGEN_DOWNLOAD_RETRY_WAIT`.
- A source that a failed run left in the work area is reused if it still matches
  the published hash, so a failed import does not cost the download again. A run
  that placed the database removes the source and the import output, so the work
  area does not grow with the number of runs.
- A run interrupted while replacing the served database has that slot restored by
  the next run, so the region index never advertises a database that is missing.
- The script therefore needs no attention while it fails: it retries, recovers
  and keeps the served tree consistent. What an operator still has to do is
  notice: the only signals are the container log and the exit status.

## 5. The container image

### Building it yourself

`scripts/mapgen/Dockerfile` builds a two-stage image:

- **build stage**: ubuntu:noble, minimized cmake Release build of the core
  library, the import library, the Import tool and the BasemapImport tool
  (all unneeded features disabled).
- **runtime stage**: ubuntu:noble, Import and BasemapImport binaries + their
  shared libraries (copied from the build stage, same distro), curl, jq,
  unzip, ca-certificates, the scripts, the bundled type definitions
  (`map.ost` plus the modules it includes, and `basemap.ost`, which includes
  none), non-root user `mapgen`.

The image names no library version: the libraries are staged with their
symlink chain by the build stage, so the version the image reports is the
version it was built from. Ask it which one that is:

```
docker run --rm --entrypoint /usr/local/bin/Import osmscout-mapgen:<version> --tool-version
```

```
docker build -f scripts/mapgen/Dockerfile -t osmscout-mapgen:<version> .

docker run --rm --read-only \
  -v /var/lib/osmscout-mapgen:/work \
  -v /repository:/repository \
  -v /etc/osm-maps:/config:ro \
  osmscout-mapgen:<version>
```

| Mount            | Purpose                                             |
|------------------|-----------------------------------------------------|
| `/work`          | Transient: downloads, import intermediate data      |
| `/repository`  | Repository volume: public/ served tree, private/ records + staging |
| `/config` (ro)   | imports.json + basemap.json (region index lives in public/names.json) |

#### Which identity the pass runs as

The pass writes into `/repository` and `/work`, so both have to be writable by
the identity it runs as. The orchestration takes that identity from two
environment variables:

| variable | default | meaning |
|----------|---------|---------|
| `PUID`   | `1000`  | the uid the pass runs as, and the owner of everything it writes |
| `PGID`   | `1000`  | the gid, same |

`1000:1000` is also the identity the image itself uses (`id -u` and `id -g` of
the `mapgen` user), so configuring nothing keeps the previous behaviour. A
repository bind-mounted from a directory owned by your own account then needs
no ownership change:

```
# .env next to the compose file, or exported in the shell
PUID=1001
PGID=1001
docker compose -f scripts/mapgen/docker-compose.yml up -d
```

A named volume is created by Docker and therefore starts out owned by root;
seed it once with the same ids (the default is shown):

```
docker compose -f scripts/mapgen/docker-compose.yml run --rm --user root \
  --entrypoint sh mapgen -c 'chown -R ${PUID:-1000}:${PGID:-1000} /repository /work'
```

For a plain `docker run`, the equivalent of the two variables is `--user`:

```
docker run --rm --read-only --user 1001:1001 \
  -v /var/lib/osmscout-mapgen:/work \
  -v /repository:/repository \
  -v /etc/os-maps:/config:ro \
  ghcr.io/framstag/libosmscout/mapgen:latest
```

The serving container is not affected: it only reads the repository, and its own
runtime directories belong to the user of its base image.

Both writable areas - `/work` and `/repository` - have to belong to the identity
the pass runs as, in **both** modes: the single pass downloads into the work area
and places databases in the repository, and the scheduled mode also generates its
crontab in the work area. When an area does not, the container refuses to start
and says which area failed, which uid and gid it is using, and the command that
fixes the ownership - `chown` for a bind mount or for a volume Docker created as
root, or `PUID`/`PGID` set to the ids that own the mount.

#### Running it on a schedule

With `MAPGEN_CRON` set, the container runs the pass on that schedule and stays up,
so no external cron job or timer is needed:

```yaml
  mapgen:
    environment:
      MAPGEN_CRON: "0 */6 * * *"
      TZ: Europe/Berlin
    restart: unless-stopped
```

| point | behaviour |
|-------|-----------|
| expression | cron, five fields (`minute hour day-of-month month day-of-week`), e.g. `0 */6 * * *`. Seven fields add seconds in front and a year at the end (`sec minute hour dom month dow year`), e.g. `*/5 * * * * * *` for every five seconds, which is handy for trying a schedule out. A six-field expression is **not** a five-field one with seconds: it keeps the minute-first order and adds a year, so `*/5 * * * * *` means every five minutes |
| time zone | the container's `TZ`; without it the expression is evaluated in UTC |
| frequent schedules | every occurrence runs a pass, but only imports that are due are downloaded and imported, so a schedule that fires more often than the refresh gates costs a check, not an import |
| overlapping passes | one pass at a time: an occurrence arriving while a pass runs is logged as skipped, and an externally triggered pass during a scheduled one is skipped the same way, since both take the lock in the work area |
| logs | the passes log into the container log: `docker compose logs -f mapgen` |
| an unusable expression | the container exits at start with the parser's error, instead of running nothing |
| `MAPGEN_CRON` unset | one pass per start and exit, as before: `docker compose run --rm mapgen`, a host cron job or a systemd timer keep working |

`restart: unless-stopped` belongs with `MAPGEN_CRON`: a container that stays up
has to come back if it dies. The compose file takes that policy from
`MAPGEN_RESTART`, and defaults it to `no` for the single-pass mode.

The ownership requirement of the mount table applies here as well, and it is the
first thing a fresh deployment gets wrong: a named volume is created by Docker as
root, so the scheduled container refuses to start, naming the area and the
`chown` command, until `/work` and `/repository` belong to the identity the pass
runs as.

Arguments always mean "run the pass once with these arguments", whatever is
configured, so `docker compose run --rm mapgen --check-config` keeps checking
the configuration and exits.

### Published images

Every build of the images on the main branch publishes to GitHub Packages, so
an operator does not build anything:

```
docker pull ghcr.io/framstag/libosmscout/mapgen:<tag>
docker pull ghcr.io/framstag/libosmscout/mapserve:<tag>
```

| Image                              | Contents                                  |
|------------------------------------|-------------------------------------------|
| `ghcr.io/framstag/libosmscout/mapgen`   | Regeneration container (this section) |
| `ghcr.io/framstag/libosmscout/mapserve` | Read-only web server (see below)      |

| Tag | Meaning | Moves? |
|-----|---------|--------|
| `<release version>` | The version the source declares (`version:` in `meson.build`), e.g. `2026.01.15.1`. It is a milestone marker: every later build carries it too, until the next milestone changes the source. | yes, with every publication of that version |
| `latest` | The newest publication, whatever version it carries. | yes |
| `<build stamp>` | UTC date and time of one build, e.g. `20260919T143512Z`. **Pin this one** when a deployment has to be reproducible. | no |

This is a rolling scheme: there is no separate release image. A release is a
milestone that changes the version the following builds are tagged with, so
the same commit that sets a new version is the commit that publishes the new
version tag.

Older builds are pruned automatically: the newest `KEEP_BUILDS` publications
of each image (20 by default, see `.github/workflows/mapgen_image.yml`) stay
pullable, older ones are deleted together with their build stamps. `latest`
and the release version tag always point at the newest publication.

```
# what is current
ghcr.io/framstag/libosmscout/mapgen:latest

# the version the source declares
ghcr.io/framstag/libosmscout/mapgen:2026.01.15.1

# one concrete build, pinned, pullable for the next 20 publications
ghcr.io/framstag/libosmscout/mapgen:20260919T143512Z
```

A newly published package is private. Make each package public once in its
settings (`https://github.com/orgs/Framstag/packages/container/mapgen/settings`
and `.../mapserve/settings`) so that anonymous `docker pull` works. The
workflow is the same either way; a private package only requires registry
credentials when pulling.

### The CI workflow

`.github/workflows/mapgen_image.yml` builds the image and smoke-tests it
(non-root user, the version it reports, config check, single pass with the
refresh gate pre-seeded so no network is needed). Publication depends on
those checks and happens for

- a merge or direct commit to `master` that changes an input of the image
  (`scripts/mapgen/**`, `Import/**`, `libosmscout-import/**`,
  `libosmscout/**`, `stylesheets/map.ost`, `.dockerignore`, the workflow
  itself) - a commit that changes none of them cannot change the image
  content and publishes nothing, and
- a manual run with publishing requested (`workflow_dispatch`).

Pull requests never publish. After a publication the workflow prunes the
older builds of both packages.

### Preparing a release

The version the images are tagged with and the version a database records are
declared in the source, so they are set by a pull request, not by the release
workflow:

1. Merge a `chore: release <version>, library <Y>` pull request that sets
   `version:` in `meson.build` (the release version) and the library version
   in both build systems (`project(libosmscout VERSION ...)` and
   `set(OSMSCOUT_LIBRARY_VERSION ...)` in `CMakeLists.txt`,
   `libraryVersion='...'` in `meson.build`). Merging it publishes
   `:<version>`, `:latest` and a build stamp.
2. Run `.github/workflows/release.yml` with the same two values. It asserts
   that the revision declares them, refuses a library version the previous
   release already reports, builds the distribution archives and creates the
   release and its `v<version>` tag. It publishes no image: that already
   happened in step 1.

The library version is what the import tool reports and what `db.json`
records as `import.version`; it is deliberately not the release version, so
that the soname major stays `1` and two releases never report the same
version.

### Serving the repository (web server image)

A second image (`scripts/mapgen/Dockerfile.serve`, nginx-based) exposes
the public part of the repository as a plain anonymous read-only HTTP
tree. It mounts the repository volume read-only and serves exactly
`<repo>/public/` — the private part is structurally outside the served
root, so it is never reachable and no deny rules are needed:

```
docker compose -f scripts/mapgen/docker-compose.yml up -d
curl http://localhost:8080/names.json
```

The compose file orchestrates both containers on one shared repository
volume: `mapgen` (writer, one pass per start, refresh-gated) and `serve`
(reader, long-running, port 8080). The web server serves regenerated
slots immediately without restart. Contract: spec `web-server`.

By default both services build from the local sources. To run the published
images instead, name them and pull them first:

```
export MAPGEN_IMAGE=ghcr.io/framstag/libosmscout/mapgen:<tag>
export MAPSERVE_IMAGE=ghcr.io/framstag/libosmscout/mapserve:<tag>
docker compose -f scripts/mapgen/docker-compose.yml pull
docker compose -f scripts/mapgen/docker-compose.yml up -d --no-build
```

Use the same `<tag>` for both images, and use the release version rather
than `latest` when the repository has to be reproducible.

## 6. Client update check contract

A client needs exactly two files: names.json (for the UI tree) and the
db.json of its own type-config version.

```
GET <leaf-path>/v<clientTypeConfigVersion>/db.json
```

| Server state                          | Client outcome                          |
|---------------------------------------|-----------------------------------------|
| db.json, generatedAt newer than local | update available                        |
| db.json, generatedAt not newer        | up to date                             |
| db.json, no local data                | available for installation (fresh)     |
| no db.json for the client's version   | unavailable; stop, keep local state    |

Rules:

- The client probes **only its own type-config version**. It never probes
  newer versions and never offers them.
- The local baseline is the client's own last-seen db.json (its
  `generatedAt`); absence means fresh install.
- Downloaded files are verified against the crc32 values in db.json; a
  verification failure is a failed download, not usable data.
- The client can access only names.json, db.json files, and database
  files. `admin/` and `staging/` live in `private/`, structurally outside
  the served root and thus never part of any response.

The decision matrix is exercised by `scripts/mapgen/client-check-test.sh`
(registered as a ctest test). The practical integration guide for client
applications is `Documentation/MapClientGuide.md` (endpoints, decision
matrix, download and CRC-32 verification, worked examples).

### The basemap follows the same contract

The basemap is not a region, so the region index cannot name it. It has its own
entry point and its own comparison base:

```
GET basemap/index.json                              -> which versions exist
GET basemap/v<clientTypeConfigVersion>/db.json      -> metadata of one version
GET basemap/v<clientTypeConfigVersion>/<file>       -> data files
```

| Server state                                   | Client outcome                     |
|------------------------------------------------|------------------------------------|
| manifest offers a version the client can read  | available for installation         |
| manifest's version or change time is newer     | update available                   |
| installed metadata matches the manifest        | up to date                         |
| manifest missing, empty, or unparsable         | basemap unavailable, no error      |
| manifest offers only versions too new to read  | basemap unavailable, no error      |

Rules:

- A client offers the newest version in the manifest that is not newer than the
  database format version it reads. It never offers a version it cannot read.
  Both the regional listing and the basemap probe use the same format version.
- The client keeps the metadata of the version it downloaded next to the data
  and compares `typeConfigVersion` and `changedAt` against the manifest, so it
  needs no directory listing and no file name.
- Every data file is verified against the checksum in that metadata before the
  installation is registered; a mismatch discards the download and leaves the
  previous installation usable.
- The basemap is optional: none of the states above is reported as an error to
  the user.

The basemap behaviour is exercised by `scripts/mapgen/basemap-check-test.sh`
(registered as a ctest test) on the generating side, and by
`JavaScout/src/test/java/com/framstag/libosmscout/client/BasemapManagerTest.java`
on the consuming side.

## 7. Operations

### Retention

`history` (global default, per-id override) states how many type-config
versions of a database the server keeps per import. After each successful
placement the oldest version slots beyond the retention depth are removed.
This is the main disk-space knob: database files are large, and parallel
type-config versions multiply them. The basemap has its own `history` in
basemap.json and is pruned the same way; its slots are pruned before the
manifest is rewritten, so the manifest never names a version that is gone.

### Refresh

`refresh` (global default, per-id override) states the minimum days
between change checks per import. The check itself is a single small HTTP
GET of the md5 sidecar; the full download and import happen only when the
published hash differs from the last imported one.

The basemap has two intervals in basemap.json: `refresh` for how often the
planet export may be hashed and the coastline source asked (a conditional
request, so an unchanged source transfers nothing), and `coastlinesRefresh`
for how often a newer coastline copy may be adopted. The upstream coastline
data changes about daily; the adoption interval is what keeps the basemap from
being rebuilt that often. A pass inside `refresh` does no work for the basemap
at all, and a pass that finds newer coastline data before adoption is due
reports it and keeps serving the adopted copy.

### Publishing

The repository volume is split at the top level into `public/` (served)
and `private/` (never served). To publish to a separate web server,
replicate only the public part; no exclude rules are needed because the
private part is structurally outside the served root:

```bash
rsync -a --delete /repository/public/ webhost:/srv/web/
```

Run after a successful pass, or on an independent schedule. The hosted
tree then contains exactly names.json and the version slots. Version
slots are immutable after publication except for same-version
regeneration; clients verify per-file crc32 against db.json, so a torn
transfer is detected, not trusted. As defense in depth, the private part
may be `chmod 700`; this is optional because it is never part of the
replication source.

### Rollback

The change is additive: the old Import binary still runs (the new
parameters are optional, the emitter is a post-success step). Reverting to
an older image/binary leaves existing databases valid; db.json is
recreated on the next import.

## 8. Files

| Path                                   | Purpose                                   |
|----------------------------------------|-------------------------------------------|
| `libosmscout/include/osmscout/io/Crc32.h` | Public CRC-32 API (`osmscout::Crc32`, `osmscout::ComputeFileCrc32`) |
| `libosmscout/src/osmscout/io/Crc32.cpp` | CRC-32 implementation (zlib-compatible)   |
| `Import/src/Import.cpp`                | CLI args, emit step invocation            |
| `libosmscout-import/include/osmscoutimport/JsonWriter.h` | Minimal write-only JSON writer |
| `libosmscout-import/include/osmscoutimport/DbJson.h` | db.json read/write, file inventory |
| `libosmscout-import/src/osmscoutimport/{JsonWriter,DbJson}.cpp` | their implementation |
| `scripts/mapgen/mapgen.sh`              | Regeneration script                       |
| `scripts/mapgen/mapgen-basemap.sh`      | Basemap step (inputs, production, placement, manifest) |
| `scripts/mapgen/imports.example.json`  | Example F1                                |
| `scripts/mapgen/names.example.json`    | Example F2                                |
| `scripts/mapgen/basemap.example.json`  | Example F4                                |
| `scripts/mapgen/nginx.example.conf`     | Example webserver config (deny rules)     |
| `scripts/mapgen/Dockerfile.serve`       | Read-only static web server image          |
| `scripts/mapgen/nginx-serve.conf`       | Served-root config for the web server      |
| `scripts/mapgen/docker-compose.yml`     | Orchestrates mapgen + serve on one volume  |
| `Documentation/MapClientGuide.md`       | Client integration guide                   |
| `scripts/mapgen/client-check-test.sh`  | Client decision-matrix test harness       |
| `scripts/mapgen/basemap-check-test.sh` | Basemap step contract test harness        |
| `scripts/mapgen/Dockerfile`            | Container image                          |
| `.github/workflows/mapgen_image.yml`   | Image build + smoke test CI job          |
| `Tests/src/JsonWriterTest.cpp`         | JSON writer unit tests                   |
| `Tests/src/DbJsonWriterTest.cpp`      | db.json read/write + CRC-32 unit tests    |
