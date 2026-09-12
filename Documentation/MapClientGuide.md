# Map Repository — client integration guide

This guide explains how a client application (mobile app, desktop app, tile
server) detects, downloads, and verifies offline map databases from a
libosmscout map repository. The normative contract is the OpenSpec change
`map-meta-structure` (`client-update-check`, `imports-manifest`,
`region-index`, `map-metadata`); this document is the practical guide.

## 1. What a client needs

Exactly two files per interaction:

| File | Purpose |
|------|---------|
| `names.json` | region index: the UI tree of available regions |
| `<leaf-path>/v<typeConfigVersion>/db.json` | metadata of one database (the client probes **only its own type-config version**) |

Everything else — the database data files — is downloaded afterwards, using
the inventory in db.json.

Conventions:

- The server is a plain read-only web server; no authentication.
- The client's type-config version is the `FILE_FORMAT_VERSION` it was
  compiled with. It never probes other versions.
- If the server does not have the client's version, the client treats the
  region as unavailable and keeps its local state.

## 2. Discovery — names.json

`GET <base-url>/names.json`

```json
{
  "schema": 1,
  "regions": [
    {
      "id": "europe",
      "names": {"en": "Europe", "de": "Europa"},
      "children": [
        {
          "id": "germany",
          "names": {"en": "Germany", "de": "Deutschland"},
          "children": [
            {"id": "berlin", "names": {"en": "Berlin"}}
          ]
        }
      ]
    }
  ]
}
```

- `schema` must equal the supported version; refuse unknown values.
- Internal nodes have `children`; leaf nodes reference import ids and are
  the downloadable databases.
- Leaf ids are keys into the database slots: a leaf `berlin` under
  `europe/germany` lives at `europe/germany/berlin/...` on the server.

## 3. Update check — db.json of the client's own version

`GET <base-url>/<leaf-path>/v<clientTypeConfigVersion>/db.json`

Interpretation:

| Server state | Client outcome |
|---|---|
| db.json, `generatedAt` newer than local baseline | update available |
| db.json, `generatedAt` not newer than local baseline | up to date |
| db.json, no local baseline (fresh install) | available for installation |
| HTTP 404 (no db.json for client's version) | unavailable; stop, keep local state |

Rules:

- The client probes **only its own type-config version**, even if only
  newer versions exist on the server. It never offers newer versions.
- The local baseline is the client's own last-seen db.json (`generatedAt`);
  absence means fresh install.
- Do not treat other HTTP errors (timeout, 5xx) as "unavailable for
  installation" — they are transient failures; retry later.

Example db.json (subset):

```json
{
  "schema": 1,
  "typeConfigVersion": 27,
  "generatedAt": "2026-09-12T07:33:59Z",
  "source": {"url": "https://...", "md5": "..."},
  "output": {
    "boundingBox": {"minLon": 6.78, "minLat": 50.67, "maxLon": 9.35, "maxLat": 52.24},
    "files": {
      "map.lib":   {"size": 12345678, "crc32": 2912136757},
      "nodes.dat": {"size": 987654321, "crc32": 316299821}
    }
  }
}
```

## 4. Download and verification

1. Take the file list from `output.files` (relative file names).
2. Download each file as `<base-url>/<leaf-path>/v<version>/<file>`.
3. For each file verify:
   - byte size equals `size`, and
   - CRC-32 equals `crc32`.

CRC-32 is the standard IEEE 802.3 CRC-32, bit-identical to zlib's
`crc32()` (and to `osmscout::ComputeFileCrc32` in the libosmscout library).
**Do not use POSIX `cksum`** — it uses a different CRC parametrization.

```sh
# verify a download with python zlib
python3 - <<'EOF'
import zlib, json, sys, os
slot, db = sys.argv[1], json.load(open(sys.argv[2]))
for f, v in db["output"]["files"].items():
    data = open(os.path.join(slot, f), "rb").read()
    assert len(data) == v["size"], (f, "size")
    assert (zlib.crc32(data) & 0xFFFFFFFF) == v["crc32"], (f, "crc32")
print("verify OK")
EOF
```

A failed verification is a **failed download** — the data is discarded, not
used. The client may retry the download; it must never fall back to
unverified data.

## 5. Worked example (curl)

```sh
BASE=https://maps.example.org

# 1. region index (UI tree)
curl -fsS "$BASE/names.json"

# 2. update check for own type-config version, e.g. 27
curl -fsS "$BASE/europe/germany/berlin/v27/db.json"   # 200 = available / 404 = unavailable

# 3. download a database slot (files from db.json output.files)
mkdir -p slot && cd slot
for f in map.lib nodes.dat ways.dat; do
  curl -fsS "$BASE/europe/germany/berlin/v27/$f" -o "$f"
done
# verify sizes + crc32 as in section 4
```

## 6. Reference

- Contract specs: `openspec/changes/map-meta-structure/specs/`
  (`client-update-check/`, `map-metadata/`, `region-index/`,
  `imports-manifest/`)
- Pipeline documentation: `Documentation/MapRepository.md`
- Server layout: served root contains exactly `names.json` and version
  slots; `private/` is structurally outside the served root and never
  reachable.
- Verification helpers: `libosmscout/include/osmscout/io/Crc32.h`
  (`osmscout::Crc32`, `osmscout::ComputeFileCrc32`)
- Client decision-matrix harness: `scripts/mapgen/client-check-test.sh`
