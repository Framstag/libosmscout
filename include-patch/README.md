# include-patch

This directory contains symlinks and header patches needed to work around
inconsistencies in third-party library installations.

## Why this exists

Some system-installed libraries ship headers with relative `#include` paths
that don't match the actual filesystem layout. Rather than modifying the
installed headers (which would break on upgrade), we add this directory to
the compiler's include path and place symlinks here to fill the gaps.

## Current patches

| Symlink | Target | Reason |
|---------|--------|--------|
| `modules/skcms` → `/usr/include/skcms` | Skia's `SkColorSpace.h` includes `"modules/skcms/skcms.h"` but the file is installed at `/usr/include/skcms/skcms.h` |

## Adding a new patch

1. Create the directory structure under `include-patch/` matching the
   `#include` path the compiler expects
2. Symlink to the actual file or directory
3. Ensure the parent directory is added to `target_include_directories`
   in the relevant `CMakeLists.txt` (already done for Skia)
