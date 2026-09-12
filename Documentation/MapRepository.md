# Map Repository — generation, metadata, and client update contract

This document describes the complete map repository pipeline: how maps are
generated from OSM extracts, what metadata is produced, how the repository
is laid out on the server, how the regeneration script and container work,
and how clients detect updates.

The pipeline is defined by the OpenSpec change `map-meta-structure`
(`openspec/changes/map-meta-structure/`). The specs there are the normative
contract; this document is the operational guide.

```
+--------------------------------------------------------------+
|  OVERVIEW                                                     |
+--------------------------------------------------------------+
|                                                              |
|  imports.json (F1)   names.json (F2)                         |
|  id -> download URL  id -> localized names + hierarchy       |
|        |                    |                                |
|        v                    v                                |
|  +---------------------------------------+                   |
|  |  mapgen.sh (regeneration script)      |                   |
|  |  curl -> md5 verify -> Import ->      |                   |
|  |  place -> prune -> records            |                   |
|  +---------------------------------------+                   |
|        |                                                    |
|        v                                                    |
|  /repository (repository volume)                          |
|    public/  names.json, <region-index-path>/v<version>/*     |
|    private/ admin/ (records), staging/ (atomic swap)          |
|        |                                                    |
|        v                                                    |
|  client: GET names.json -> probe own version -> compare      |
|          generatedAt -> download -> verify crc32             |
+--------------------------------------------------------------+
```

## 1. The three data files

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
  private/                           never served (outside served root)
    admin/
      berlin_v27_generation.json     per-slot run records
      berlin_check.json              check-cycle state
    staging/
      berlin/v27.new/                staging for atomic replacement
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
- The client-accessible surface is exactly: names.json, per-database
  db.json, and database files.

## 3. Integrity model

Two layers, each fit for its purpose:

| Layer   | What it protects            | Mechanism                                        |
|---------|-----------------------------|--------------------------------------------------|
| Source  | authenticity of the extract | md5 sidecar published by the download service, verified by the script with `md5sum` |
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
| `MAPGEN_TYPEFILE`      | `map.ost`        | Type definition file                    |
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

### Script-owned state (private/admin/)

| File                          | Content                                        |
|-------------------------------|------------------------------------------------|
| `<id>_check.json`             | `lastCheckedAt` (epoch), `lastSeenSourceMd5`    |
| `<id>_v<version>_generation.json` | placement time, server path, retention, pruned versions, source change result, run outcome |

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

## 5. The container image

`scripts/mapgen/Dockerfile` builds a two-stage image:

- **build stage**: ubuntu:noble, minimized cmake Release build of the core
  library, the import library, and the Import tool (all unneeded features
  disabled).
- **runtime stage**: ubuntu:noble, Import binary + its shared libraries
  (copied from the build stage, same distro), curl, jq, ca-certificates,
  the script, the bundled `map.ost`, non-root user `mapgen`.

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
| `/config` (ro)   | imports.json (region index lives in public/names.json) |

The image tag should equal the libosmscout version; db.json records
`import.version`, so tree drift from an older binary is visible.

The CI workflow `.github/workflows/mapgen_image.yml` builds the image and
smoke-tests it (non-root user, config check, single pass with the refresh
gate pre-seeded so no network is needed).

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

## 7. Operations

### Retention

`history` (global default, per-id override) states how many type-config
versions of a database the server keeps per import. After each successful
placement the oldest version slots beyond the retention depth are removed.
This is the main disk-space knob: database files are large, and parallel
type-config versions multiply them.

### Refresh

`refresh` (global default, per-id override) states the minimum days
between change checks per import. The check itself is a single small HTTP
GET of the md5 sidecar; the full download and import happen only when the
published hash differs from the last imported one.

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
| `Import/src/JsonWriter.{h,cpp}`         | Minimal write-only JSON writer            |
| `Import/src/DbJsonWriter.{h,cpp}`       | db.json emission                          |
| `scripts/mapgen/mapgen.sh`              | Regeneration script                       |
| `scripts/mapgen/imports.example.json`  | Example F1                                |
| `scripts/mapgen/names.example.json`    | Example F2                                |
| `scripts/mapgen/nginx.example.conf`     | Example webserver config (deny rules)     |
| `scripts/mapgen/Dockerfile.serve`       | Read-only static web server image          |
| `scripts/mapgen/nginx-serve.conf`       | Served-root config for the web server      |
| `scripts/mapgen/docker-compose.yml`     | Orchestrates mapgen + serve on one volume  |
| `Documentation/MapClientGuide.md`       | Client integration guide                   |
| `scripts/mapgen/client-check-test.sh`  | Client decision-matrix test harness       |
| `scripts/mapgen/Dockerfile`            | Container image                          |
| `.github/workflows/mapgen_image.yml`   | Image build + smoke test CI job          |
| `Tests/src/JsonWriterTest.cpp`         | JSON writer unit tests                   |
| `Tests/src/DbJsonWriterTest.cpp`      | db.json + CRC-32 unit tests              |
