# Design

## Context

See `proposal.md` - Why. The state that shapes the approach, all in `scripts/mapgen/mapgen.sh`:

- `process_import` (around line 360) decides what to do with a source file that an earlier
  run left in the work area (`$pbf = $WORK_DIR/$id.<ext>`):
  ```sh
  if [[ -f "$pbf" ]] && (cd "$WORK_DIR" && printf '%s  %s\n' "$published_hash" "$pbf" | md5sum -c -); then
    log "$id: reusing the verified source from the work area"
  elif ! download_source "$url" "$pbf"; then
  ```
  The reuse branch is content-based and therefore correct. The fall-through is the bug: it
  treats every non-matching file as a partial of the source being fetched.
- `download_source` (around line 245) then transfers with `curl -C - -o "$pbf"`, which
  appends to whatever the file already holds. On a complete file of an older version, curl
  resumes from that file's size and appends the remainder of the *new* remote file: the
  result is neither version.
- The verification step (around line 379) then fails and discards the file, so the failure
  is self-healing - which is why the defect is a wasted download and a misleading log
  line rather than a stuck deployment.
- What the work area keeps, and why, is specified: a successful run removes the source
  (`rm -f "$pbf"` at the end of `process_import`), a failed **import** keeps it "for a later
  attempt to resume" (`openspec/specs/regen-script/spec.md`, "Recovery from interrupted
  work"). A failed import and an interrupted download therefore leave files that look
  identical to the check that follows.
- The failure-recovery check (`scripts/mapgen/recovery-check-test.sh`) already drives the
  script without a network: a fixture source with its md5 sidecar, a stub import tool that
  can succeed or fail on demand, and log-string assertions. It is run by
  `.github/workflows/mapgen_image.yml` inside the image, and can be run by hand with
  `MAPGEN_SCRIPT` pointing at the script under test.

Observed incident that motivated this change: a source of `nordrhein-westfalen` (912 MB)
was left by a failed import, the source was re-published, and the next pass resumed onto
the kept file in 9 seconds and reported a verification failure. The file was the mixture
described above, not a corrupt download.

## Goals / Non-Goals

**Goals:**

- A kept source file is resumed if and only if it belongs to the source being fetched.
- The case that produced the incident is reproducible in the failure-recovery check, and a
  partial of the current source stays resumable.
- No new state outside the work area, and no new configuration.

**Non-Goals:**

- Verifying a kept file against a *previous* published hash (that would need the hash of
  every version ever seen; the record only distinguishes "this source" from "not this
  source").
- Changing the `refresh` gating, the work-area cleanup rules, or what a failed import keeps.
- Making the resume decision from the server (`Content-Length`, `ETag`, `Last-Modified`):
  see D1, alternative B.
- Touching the basemap step, which downloads no source by hash and caches its import output
  by extract content instead (`scripts/mapgen/mapgen-basemap.sh`).

## Decisions

### D1 - A record beside the kept file names the source it belongs to

Chosen: when a transfer starts, the published hash it is fetching is written to
`<pbf>.source-md5` next to the file. The resume decision asks that record, not the file
content: a file is resumable when the record exists, is present for a file that exists,
and equals the published hash being fetched now. A file without a matching record is
discarded (`rm -f "$pbf" "$pbf.source-md5"`) before the transfer.

- Alternative A - keep the content-only decision and delete the file whenever it does not
  match the published hash. Rejected: that throws away a partial download of exactly the
  source being fetched, which is the case resume exists for; on a 900 MB extract, a
  network that drops repeatedly would restart from zero on every attempt.
- Alternative B - ask the server whether the kept file is a prefix of the current source
  (`Content-Length` comparison, `Range: bytes=<size>-` probe with `If-Range`). Rejected: it
  needs a second request per attempt, Geofabrik re-publishes under the same URL so no
  validator is stable across versions, and the answer is only as good as the server's
  headers - the local record needs no network at all and is exact for the question asked.
- Alternative C - a separate state file per import under `private/admin/`
  (e.g. `<id>_download.json`). Rejected: the record must disappear with the file it
  describes, and the work area is the area that is allowed to be discarded (a pass that
  starts with a fresh work area must lose nothing but work). Two files that have to be
  removed together in five places are also easier to get wrong than one naming convention.
- Alternative D - hash the kept file and look the hash up in the generation records.
  Rejected: a partial has no record, and the import records only exist for placed
  databases, so this answers the question for exactly the case that is already handled by
  the content check.

### D2 - The record is written before the transfer, and the discard happens before the call

Chosen: `download_source` writes the record immediately before its attempt loop, and
`process_import` discards a foreign file before calling `download_source`. Both are on the
side of the boundary that can observe the condition:

- The record has to precede the transfer, not follow it: the run that is killed mid-
  download leaves a partial with no other trace, and it is that partial the next run has to
  resume. Writing it after a successful transfer would cover a complete file (already
  handled by the content check) and miss every partial.
- The discard belongs to the decision in `process_import`, which knows `$id` (the log line
  names the import) and has already run the reuse check. `download_source` is then left
  with one responsibility - fetch, retry, resume - and keeps its own `rm -f` for the
  "service cannot resume" branch only, where the file is a mixture of the current source
  by construction.

### D3 - The record is removed with the file, and nowhere else

Chosen: all removals of `$pbf` go through one helper that removes the record too - after a
successful placement, after a verification failure, and in the foreign-source discard (the
`curl` status 33 branch keeps only the file, since the record is still the current source's
and a following attempt may leave a partial to resume).

- Alternative A - a work-area sweep that removes orphan records at pass start. Rejected:
  the record of a *missing* file is harmless (the resume check requires the file), and a
  sweep adds a directory walk to every pass for no behaviour.

## Flow

```
pass (process_import, source changed)
  |
  |-- md5(kept $pbf) == published hash?  --yes--> reuse: import from $pbf
  |        |no
  |        |
  |        |-- record beside $pbf exists and == published hash?  --no--> discard $pbf (+record)
  |        |        |yes                                                    |
  |        |        v                                                       v
  |        |     keep $pbf (partial of this source)               download_source
  |        |        |                                            - write record = published hash
  |        |        |                                            - curl -C - (resume or fresh)
  |        |        |                                            - retry within the run, on
  |        |        |                                              curl 33 restart from zero
  |        |        v                                                       |
  |        +------->+-------------------------------------------------------+
  |                                  |
  |                    md5($pbf) == published hash?  --no--> discard $pbf (+record), fail import
  |                                  |yes
  |                                  v
  |                             run Import, place, discard $pbf (+record)
```

## Risks

- **A partial is no longer resumed, if the record is ever not written.** The record is
  written unconditionally at the start of `download_source`, before the attempt loop, and
  the check that consumes it requires the file to exist, so a missing record costs one
  full download and never a wrong file. Covered by the second new test case, which pins
  the resume of a partial of the same source.
- **A record that outlives its file.** Only reachable with a work area that is modified by
  hand; harmless by the same argument (a resume requires the file).
- **A stale record matching while the file is foreign.** Requires the work area to be
  manipulated outside the script; the content check that runs first would still reject the
  file for a changed source, and the verification after the transfer is unaffected.
- **The file name carries a `.source-md5` suffix next to a file the script removes.** The
  import tool never sees the work area, and the record is not a `.pbf`, so nothing that
  globs sources by extension picks it up.
