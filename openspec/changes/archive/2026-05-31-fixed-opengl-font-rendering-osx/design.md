## Context

Text rendering in the OpenGL backend uses FreeType, which expects individual Unicode codepoints (via `FT_Get_Char_Index(face, codepoint)`). The current code calls `UTF8StringToU32String` to decode UTF-8 to a `char32_t` sequence. This function uses `std::codecvt_utf8<char32_t>` with default mode, which on macOS (libc++) produces incorrect codepoint values for multi-byte sequences — the byte order within each 32-bit unit is platform-dependent.

The old iconv-based code worked because it explicitly specified `"UTF-32LE"` as the target encoding. The switch to `std::codecvt_utf8` lost that explicit byte-order constraint.

No other renderer backend is affected — only the OpenGL backend uses `UTF8StringToU32String` for FreeType consumption.

## Goals / Non-Goals

**Goals:**
- Fix garbled text on macOS OpenGL backend
- Add a new standard-library-based conversion that always produces little-endian `char32_t`
- Keep existing `UTF8StringToU32String` unchanged for other callers
- Zero new library dependencies

**Non-Goals:**
- No changes to other renderer backends
- No changes to FreeType integration itself
- No hand-rolled UTF-8 parsing
- No changes to `std::u32string` memory representation on the target machine (this is a codepoint-value issue, not an in-memory byte-order issue)

## Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Fix location | New function, not modify existing | Existing callers may depend on native-byte-order behavior. Separation preserves correctness for all callers. |
| Conversion method | `std::codecvt_utf8<char32_t, 0x10ffff, std::little_endian>` | Matches old iconv `UTF-32LE` target. Pure standard library — no new deps. libstdc++ and libc++ both support the `little_endian` flag. |
| MSVC path | Same template with `int32_t` + `reinterpret_cast` | MSVC `codecvt_utf8<char32_t>` linker issue requires the workaround. `little_endian` flag works the same way on this path. |
| OpenGL call site | Replace `UTF8StringToU32String(text)` with `UTF8StringToU32StringLE(text)` | Minimal diff, no architectural change. |
| Naming | `UTF8StringToU32StringLE` | Explicit, self-documenting. Follows existing naming convention in String.h. |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| MSVC workaround fails with `std::little_endian` on `int32_t` path | Test on MSVC. The `int32_t` vs `char32_t` is just the element type for the linker workaround — `little_endian` flag is orthogonal and should compose. |
| Another platform uses non-LE expectation | Unlikely — OpenGL rendering is inherently LE (texture data bytes). If needed, add `UTF8StringToU32StringBE` variant. |
| `std::codecvt_utf8` deprecated in C++17/removed in future standard | Already the status quo. When removed, both old and new function need replacement — same migration burden. |
