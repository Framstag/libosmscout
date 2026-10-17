# Design

## Root cause

`libosmscout/src/osmscout/util/StringMatcher.cpp` includes `<algorithm>` after its project headers while
using nothing from it: the header list carries one dependency the translation unit does not have, in an
order the code style forbids. The entry names both halves; the file's own content proves the second - the
only `std::` names the file uses are `std::string`, `std::string::npos`, `std::vector` and
`std::make_shared`, all of them provided by `libosmscout/include/osmscout/util/StringMatcher.h`
(`<memory>`, `<string>`, `<vector>` at lines 23-25) and `libosmscout/include/osmscout/util/String.h`. The
search of `evidence/symbol-audit.log` looks for every algorithm name the file could use and finds none.

## The single fix

Delete the `#include <algorithm>` line and the blank line that separated it from
`<osmscout/util/String.h>`. Nothing else in the file changes.

The alternative - moving the include into the standard-library block, between the own header and
`<osmscout/util/String.h>` - is eliminated by the evidence, not by taste: the file needs no name from
`<algorithm>`. Relocating would satisfy the order half of the defect and keep the unused-dependency half,
so it is not a second viable fix. Removing the line settles both halves ("removal preferred" in the
backlog entry).

The check is scoped to this one file on purpose. The rule of `guidelines/CodeStyles.md` "Include Order" is
violated in 113 files in the six measured library directories - the count of `evidence/residue-out-of-order-includes.log`, among them
`libosmscout/include/osmscout/Pixel.h:30` (which lists `<osmscout/system/Compiler.h>` before its
standard-library block at lines 32-36) and
`libosmscout-client/src/osmscoutclient/FavoriteLocationService.cpp:24` (which lists `<algorithm>` after
`<osmscoutclient/json/json.hpp>`). A tree-wide check would fail on all of them at once - a formatting pass of
its own, not the one-line cleanup §89 asks for. Phase A's measurement of that drift is retained in that log
and filed as a separate `TODO.md` entry.

The check is textual because the defect is a property of the file's text, not of the program's behaviour: a
compiler accepts the unused include silently, and no run of the library can observe it. The scan reuses
`HeaderCheckTest`'s include machinery (`SOURCE_ROOT`, `GetIncludes`-style regex and stderr reporting) so that
no new binary, no registration and no build-description change is needed. The case prints the figure it
measures (`STD_INCLUDES_AFTER_PROJECT=`), so the number survives in the runner's log and is re-checkable.

Usage of a header is not decidable textually in general, so the invariant the suite owns is the order - the
half the file's text can prove. The unused half is proven by the symbol audit above, retained in this design
and in `evidence/red-head-check.log`.

## Blast radius

- One translation unit of `libosmscout` loses an unused header: no symbol, no signature, no macro and no
  declaration used by any other file is touched, and no other translation unit includes
  `StringMatcher.cpp`.
- One test translation unit gains a helper and a case; both build systems already compile and register the
  binary, so no `Tests/CMakeLists.txt` or `Tests/meson.build` change is needed.
- No public header, no type configuration, no serialized layout, no database: `guidelines/FileFormatVersion.md`
  asks for no version bump.
- `HeaderCheckTest` grows from 2 to 3 cases; the two existing cases ("Check format of includes", "Check
  include dependencies") still run over the same file list.

## Rollback

Re-add the two removed lines (`#include <algorithm>` below the blank line after
`<osmscout/util/String.h>`); the case of the check then goes red again, which is exactly what the probe
records. The check can be dropped by deleting the case and its helper; nothing else depends on them.

## Sequence

Not applicable - a textual edit of one include block and one host check, no runtime flow.

## Risks

- A check that reads the source tree needs `SOURCE_ROOT`; both build systems set it (CMake:
  `Tests/CMakeLists.txt:170`; Meson: `headerCheckEnv` in `Tests/meson.build:391`), so a run started without
  it fails on `CHECK(sourceRoot!= nullptr)` instead of silently passing. `CHECK(exists(file))` fails if the
  file is moved, so a rename cannot make the case vacuously green.
- The check covers one file; the same drift elsewhere stays unobserved until the filed entry is processed.
  That is stated, not hidden.
- Formatter conformance: `Tests/src/HeaderCheckTest.cpp` carries pre-existing uncrustify drift - the drift
  of `evidence/uncrustify.log` is the two hunk ranges `@@ -10,25 +10,25 @@` and `@@ -53,413 +53,422 @@`; the
  second range runs to `Tests/src/HeaderCheckTest.cpp:465`, two lines into the comment of the block this
  change adds, so the added test block (lines 464-532) is itself unreformatted. `libosmscout/src/osmscout/util/StringMatcher.cpp` drifts in 58 diff lines, the
  pre-existing drift `TODO.md` §88 already records for that file; deleting a line cannot add a hunk, and a
  full formatting pass is that entry's subject, not this change's.
