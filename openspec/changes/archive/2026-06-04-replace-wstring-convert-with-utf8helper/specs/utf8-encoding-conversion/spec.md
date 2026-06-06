## ADDED Requirements

### Requirement: UTF-8 to wstring conversion

The system SHALL convert an UTF-8 encoded `std::string` to `std::wstring` without using `std::wstring_convert` or `<codecvt>`.

#### Scenario: ASCII input round-trips correctly

- __WHEN__ the input is an ASCII string "abcABC"
- __THEN__ converting to `std::wstring` and back to UTF-8 via `WStringToUTF8String` SHALL yield the original string

#### Scenario: Multi-byte input round-trips correctly

- __WHEN__ the input contains multi-byte UTF-8 sequences (e.g., German umlauts "\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc" and euro sign "\xe2\x82\xac")
- __THEN__ converting to `std::wstring` and back to UTF-8 via `WStringToUTF8String` SHALL yield the original string

#### Scenario: 4-byte codepoint round-trips correctly

- __WHEN__ the input contains a 4-byte UTF-8 sequence (e.g., emoji "\xf0\x9f\x8d\x8c")
- __THEN__ converting to `std::wstring` and back to UTF-8 SHALL yield the original string

#### Scenario: Empty string returns empty wstring

- __WHEN__ the input is an empty string
- __THEN__ the result SHALL be an empty `std::wstring`

### Requirement: UTF-8 to u32string conversion

The system SHALL convert an UTF-8 encoded `std::string` to `std::u32string` without using `std::wstring_convert` or `<codecvt>`.

#### Scenario: ASCII input yields correct u32string

- __WHEN__ the input is "test"
- __THEN__ the result SHALL be a `std::u32string` of size 4 with characters 't', 'e', 's', 't'

#### Scenario: Empty string returns empty u32string

- __WHEN__ the input is an empty string
- __THEN__ the result SHALL be an empty `std::u32string`

### Requirement: UTF-8 to u32string little-endian conversion

The system SHALL convert an UTF-8 encoded `std::string` to `std::u32string` with explicit little-endian byte order, without using `std::wstring_convert` or `<codecvt>`.

#### Scenario: ASCII input yields correct LE u32string

- __WHEN__ the input is "test"
- __THEN__ the result SHALL be a `std::u32string` of size 4 with characters 't', 'e', 's', 't'

#### Scenario: 2-byte codepoint is correctly decoded

- __WHEN__ the input is the 2-byte sequence "\xc3\x9f" (U+00DF, sharp s)
- __THEN__ the result SHALL be a `std::u32string` of size 1 with value `char32_t(0xDF)`

#### Scenario: 3-byte codepoint is correctly decoded

- __WHEN__ the input is the 3-byte sequence "\xe6\xb0\xb4" (U+6C34, water CJK)
- __THEN__ the result SHALL be a `std::u32string` of size 1 with value `char32_t(0x6C34)`

#### Scenario: 4-byte codepoint is correctly decoded

- __WHEN__ the input is the 4-byte sequence "\xf0\x9f\x8d\x8c" (U+1F34C, banana emoji)
- __THEN__ the result SHALL be a `std::u32string` of size 1 with value `char32_t(0x1F34C)`

#### Scenario: Mixed multi-byte input is correctly decoded

- __WHEN__ the input is "a\xc3\x9f\xe6\xb0\xb4\xf0\x9f\x8d\x8c"
- __THEN__ the result SHALL be a `std::u32string` of size 4 with values `char32_t('a')`, `char32_t(0xDF)`, `char32_t(0x6C34)`, `char32_t(0x1F34C)`

#### Scenario: Empty string returns empty u32string

- __WHEN__ the input is an empty string
- __THEN__ the result SHALL be an empty `std::u32string`

### Requirement: wstring to UTF-8 conversion

The system SHALL convert `std::wstring` to UTF-8 encoded `std::string` without using `std::wstring_convert` or `<codecvt>`.

#### Scenario: wstring round-trips through UTF-8

- __WHEN__ a `std::wstring` with ASCII content is converted to UTF-8
- __THEN__ converting back to `std::wstring` via `UTF8StringToWString` SHALL yield the original wstring

### Requirement: No codecvt dependency

The system SHALL NOT require `<codecvt>` header or `HAVE_CODECVT` configuration flag for building.

#### Scenario: Build succeeds without codecvt

- __WHEN__ the project is built on a platform without `<codecvt>` (e.g., C++26 toolchain)
- __THEN__ the build SHALL succeed and all encoding conversion tests SHALL pass
