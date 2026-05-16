# C++ Coding Style — libosmscout

Derived from analyzing headers and implementation in `libosmscout/` core library (GeoCoord.h, Tag.h, TypeConfig.h, ObjectRef.h, TypeConfig.cpp, OSMScoutTypes.h, Compiler.h, Assert.h, NameFeature.h, FileScanner.h).

## Naming

| Element | Convention | Example |
|---------|-----------|--------|
| Classes | PascalCase | `GeoCoord`, `TypeConfig`, `FileScanner` |
| Methods | PascalCase, verbs preferred | `GetLat()`, `SetName()`, `IsValid()`, `Invalidate()` |
| Member variables | camelCase | `lat`, `lon`, `name`, `canBeNode`, `featureMaskBytes` |
| Constants | PascalCase or uppercase | `maxRawCoordValue`, `coordByteSize`, `FILE_TYPES_DAT` |
| Enums | PascalCase type, lowercase values | `enum OSMRefType { osmRefNone, osmRefNode, ... }` |
| Enum class | PascalCase type, PascalCase values | `enum class SpecialType : uint8_t { none, multipolygon, ... }` |
| Type aliases | PascalCase with `Ref` suffix | `using TypeInfoRef = std::shared_ptr<TypeInfo>;` |
| Template params | PascalCase | `template<std::size_t FlagCnt>` |
| Macros | `OSMSCOUT_*` uppercase | `OSMSCOUT_API`, `CLASS_FINAL` |

## Namespace

- Single top-level namespace: `osmscout`
- Opening brace on line after `namespace` declaration
- Content indented 2 spaces inside namespace block

```cpp
namespace osmscout {

  class OSMSCOUT_API GeoCoord CLASS_FINAL
  {
  public:
    GeoCoord() = default;
    ...
  };

} // namespace osmscout
```

## Indentation & Braces

- **2-space** indentation. No tabs.
- **Class declarations**: opening brace on its **own line**
- **Method definitions** (in .cpp): opening brace on **own line**
- **Control flow**: opening brace on **same line**
- **Access specifiers**: one indent level, no extra line before method groups

```cpp
// Class: brace own line
class OSMSCOUT_API FeatureValueBuffer CLASS_FINAL
{
private:
  TypeInfoRef type;
  uint8_t     *featureBits=nullptr;

public:
  FeatureValueBuffer() = default;
  FeatureValueBuffer(const FeatureValueBuffer& other);

  void Set(const FeatureValueBuffer& other);

  // Control flow: brace same line
  bool HasFeature(size_t idx) const
  {
    size_t featureBit=type->GetFeature(idx).GetFeatureBit();
    return (featureBits[featureBit/8] & (1u << featureBit%8))!=0;
  }

  // Template: same-line brace
  template<class T> const T* findValue() const
  {
    for (const auto& featureInstance :GetType()->GetFeatures()) {
      if (HasFeature(featureInstance.GetIndex())) {
        ...
      }
    }
  }
};
```

## Member Declarations

- **In-class initializers** preferred over constructor init for primitives:
  ```cpp
  double lat = 0.0;
  std::FILE *file=nullptr;
  mutable bool hasError=true;
  ```
- **Alignment**: members aligned by name, comments right-aligned with `//!<`:
  ```cpp
  bool                                        canBeNode=false;               //!< Type can be a node
  bool                                        canBeWay=false;                //!< Type can be a way
  bool                                        canBeArea=false;               //!< Type can be a area
  ```
- **Public members** allowed in small value-type classes:
  ```cpp
  class ObjectOSMRef CLASS_FINAL {
    OSMId      id=0;    // NOLINT
    OSMRefType type=osmRefNone; // NOLINT
  };
  ```

## Classes

- **`CLASS_FINAL`**: use the macro at end of class declaration. `#define CLASS_FINAL final` (empty for SWIG)
- **Export macro**: every public class marked with library-specific macro (`OSMSCOUT_API`, etc.)
- **Defaulted special members**: use `= default` not empty bodies
- **Deleted copy/move**: explicit `= delete` for non-copyable types
- **`explicit`** keyword on single-argument constructors
- **`override`** keyword always used for virtual overrides
- **`noexcept`** on move operations where appropriate

```cpp
class OSMSCOUT_API FeatureValueBuffer CLASS_FINAL
{
public:
  FeatureValueBuffer() = default;
  FeatureValueBuffer(const FeatureValueBuffer& other);
  FeatureValueBuffer(FeatureValueBuffer&& other) noexcept;
  ~FeatureValueBuffer();

  FeatureValueBuffer& operator=(const FeatureValueBuffer& other);
  FeatureValueBuffer& operator=(FeatureValueBuffer&& other) noexcept;

private:
  TypeInfoRef type;
  uint8_t     *featureBits=nullptr;

protected:
  void SetType(const TypeInfoRef& type);
};
```

## Methods

- **Getter pattern**: `GetX()` / `SetX()` naming (not `x()` / `setX()`)
- **Boolean queries**: `IsValid()`, `CanRoute()`, `HasFeature()`, `HasValue()` — `Is`/`Can`/`Has` prefix
- **Setters return `*this`** for chaining (fluent interface pattern):
  ```cpp
  TypeInfo& SetNodeId(TypeId id)
  {
    this->nodeId=id;
    return *this;
  }
  ```
- **Const correctness**: const methods are the default for getters; non-const overloads where mutation is needed
- **Static methods**: used for factory/parse patterns: `static bool Parse(...)`, `static TypeInfoRef Read(...)`
- **Short bodies** defined inline in header; **long bodies** in `.cpp` file

## Pointers & References

- **Reference placement**: attached to type (left): `const std::string& name`
- **Pointer placement**: `uint8_t *featureBits=nullptr` — space on left, not right
- **Smart pointers**: `std::shared_ptr<T>` with `using TRef = std::shared_ptr<T>` aliases, i.e. `TypeInfoRef`, `FeatureRef`, `TagConditionRef`
- **Raw pointers** used for non-owning access to feature value buffers
- **`nullptr`** not `NULL` or `0` (except integral zero constants)

## Enums

Two patterns coexist:

**Plain enum** (more common):
```cpp
enum OSMRefType
{
  osmRefNone     = 0,
  osmRefNode     = 1,
  osmRefWay      = 2,
  osmRefRelation = 3
};

enum Vehicle: uint8_t
{
  vehicleFoot P5 1u,
  vehicleBicycle P5 2u,
  vehicleCar P5 3u
};
```

**Enum class** (newer code):
```cpp
enum class SpecialType : uint8_t {
  none         = 0,
  multipolygon = 1,
  routeMaster  = 2,
  route        = 3
};
```

## Include Order

Each header/source file follows a strict order:
1. **Own header** first (for `.cpp` files)
2. **Standard library** headers alphabetically: `<algorithm>`, `<array>`, `<cassert>`, `<cstddef>`, `<cstdio>`, `<list>`, `<memory>`, `<string>`, `<tuple>`, `<unordered_map>`, `<vector>`
3. **Project library export macro**: `<osmscout/lib/CoreImportExport.h>`
4. **Project headers** alphabetically by full path: `<osmscout/TypeConfig.h>`, `<osmscout/feature/NameFeature.h>`, `<osmscout/system/Compiler.h>`, `<osmscout/util/Parsing.h>`
5. **Platform-specific** `#ifdef` blocks at end

## Comments

- **Doxygen blocks** for public API, using `/** ... */` style
- Standard Doxygen tags: `\ingroup`, `@param`, `@return`, `@note`, `@throws`
- **Grouping markers**: `//@\{` and `//@\}` around method groups
- **Inline member docs**: `//!<` suffix on member declaration lines
- **Code comments**: `//` for implementation notes, rarely `/* */` in code

```cpp
  /**
   * \ingroup Geometry
   *
   * Anonymous geographic coordinate.
   */
  class OSMSCOUT_API GeoCoord CLASS_FINAL
  {
  private:
    double lat = 0.0;
    double lon = 0.0;

  public:
    /**
     * Return the latitude value of the coordinate
     */
    double GetLat() const
    {
      return lat;
    }
```

## Formatting Details

- **Spaces around operators**: `lat==other.lat && lon==other.lon` — no space inside empty parens, one space around binary operators (except `::`, `->`, `.`)
- **Spaces inside parens**: none — `GetFeature(idx)` not `GetFeature( idx )`
- **Colon alignment** in constructor init lists — colon on its own line, indented 2 spaces:
  ```cpp
  TypeInfo::TypeInfo(const std::string& name)
    : name(name)
  {
    // no code
  }
  ```
- **Range-for**: `for (const auto &feature : type->GetFeatures())` — space before `:`, space before and after `:`
- **Switch cases**: fall through 
- **Empty bodies**: explicit `// no code` comment inside empty braces

## Header Guards

Traditional `#ifndef` / `#define` / `#endif` pattern (not `#pragma once`):
```cpp
#ifndef OSMSCOUT_GEOCOORD_H
#define OSMSCOUT_GEOCOORD_H
...
#endif
```
Guard name: `OSMSCOUT_<MODULE>_<NAME>_H`

## Template & Modern C++

- **Templates** used sparingly, mostly for type-safe Read/Write with flag arrays
- **`using` aliases over `typedef`** for type definitions
- **`constexpr`** used for compile-time constants
- **`auto`** for complex types (casts, iterators), explicit types for primitives
- **`std::byte`** for buffer types (C++17+, reflects C++20 baseline)
- **`std::string_view`** in newer feature code parameter signatures
- **`// NOLINT`** annotations for lint suppression on intentional public members

## Error Handling

- **`assert()`** from `<cassert>` via `osmscout/system/Assert.h` for internal invariants only
- **`@throws IOException`** documented in Doxygen for file I/O methods
- Legacy code transitioning toward Status-based error codes
- No exceptions visible in headers besides I/O operations
