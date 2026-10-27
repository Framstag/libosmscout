# Proposal

## Why

Root cause: `libosmscout/src/osmscout/util/StringMatcher.cpp` carries `#include <algorithm>` (added by
`dacb067da`) after its project includes although the file uses no name from `<algorithm>`, so its include
block declares a dependency the translation unit does not have and breaks the order of
`guidelines/CodeStyles.md`, "Include Order" (standard library before project headers).

Evidence:    `Tests/src/HeaderCheckTest.cpp` case "Check include order of util/StringMatcher.cpp" fails on
HEAD - `REQUIRE( violations.size()==0 )`, expansion `1 == 0`, messages "File
'/home/tim/projects/libosmscout/libosmscout/src/osmscout/util/StringMatcher.cpp' includes a standard library
header after its project includes: '#include <algorithm>'!" and "SOURCE=... STD_INCLUDES_AFTER_PROJECT=1";
test cases: 1 | 1 failed, assertions: 3 | 2 passed | 1 failed, exit code 42, run at 2026-10-16T11:42:06Z
(UTC) with the pins of `evidence/red-head-check.log`.

Repro:       `SOURCE_ROOT=$PWD ./build/Tests/HeaderCheckTest "Check include order of util/StringMatcher.cpp"`

Spec:        source-include-order / Standard library includes precede project includes   Guideline:
guidelines/CodeStyles.md §Include Order (step 2 precedes step 4)

The include block of the file is `<osmscout/util/StringMatcher.h>`, `<osmscout/util/String.h>`,
`<algorithm>` (`libosmscout/src/osmscout/util/StringMatcher.cpp:20`-`:22` after the fix; line 24 on HEAD).
The file uses `std::string` (13 times, with `std::string::npos`), `std::vector` (2) and `std::make_shared`
(3) and no name from `<algorithm>`, as the search of `evidence/symbol-audit.log` shows: `std::string` and
`std::vector` are provided by `libosmscout/include/osmscout/util/StringMatcher.h` (`<memory>`, `<string>`,
`<vector>` at lines 23-25) and `std::make_shared` by its `<memory>`. So the header is decoration: the include
block names a dependency (step 2) after the project headers (step 4) that no line of the file needs.

## What Changes

- `libosmscout/src/osmscout/util/StringMatcher.cpp` loses the `#include <algorithm>` line - it is removed,
  not relocated: the evidence above eliminates the file's need for the header, so moving it into the
  standard-library block would keep a dependency the file does not have.
- `Tests/src/HeaderCheckTest.cpp` gains the include-order check that reads the file's include block and
  reports the standard-library includes that follow a project include, one `TEST_CASE` naming that file.
- No other source file, no test registration and no build description changes: the case lives in the
  `HeaderCheckTest` binary both build systems already register.

## Capabilities

### New Capabilities

- `source-include-order`: the order of a checked file's include block is observable, and the file named by
  the requirement is checked for standard-library includes behind its project headers.

### Modified Capabilities

- none.

## Impact

- `libosmscout/src/osmscout/util/StringMatcher.cpp`: the `#include <algorithm>` line and the blank line
  that followed it.
- `Tests/src/HeaderCheckTest.cpp`: the helper `IncludesAfterProjectHeaders` and the case "Check include
  order of util/StringMatcher.cpp".
- No change to `Tests/CMakeLists.txt` or `Tests/meson.build` (the binary and its `SOURCE_ROOT` environment
  exist in both), no change to a public header, a type configuration or a serialized layout, so
  `FILE_FORMAT_VERSION` stays as it is.
- `TODO.md` §89: the entry is closed as `fixed-by` this change in phase F, and the include-order drift of
  the rest of the tree, which the measurement of phase A uncovered, is filed as a new entry.
- One capability directory is added to `openspec/specs/`; no existing capability text is modified.
