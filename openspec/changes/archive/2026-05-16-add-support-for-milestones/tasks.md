## 1. Feature Header File

- [x] 1.1 Create `include/osmscout/feature/HighwayMilestoneFeature.h` with `HighwayMilestoneFeatureValue` class storing `distance` as `uint32_t` and `ref`, `carriageway_ref`, `marker` as `std::string`, `GetLabel()` override, Read/Write declarations, copy/compare operators

- [x] 1.2 Add `HighwayMilestoneFeature` class with `NAME` constant, `Initialize()` for tag registration, `GetName()`, `GetValueAlignment()`, `GetValueSize()`, `AllocateValue()`, `Parse()` declarations

- [x] 1.3 Add `HighwayMilestoneFeatureValueReader` type alias using `FeatureValueReader<HighwayMilestoneFeature, HighwayMilestoneFeatureValue>`

## 2. Feature Implementation File

- [x] 2.1 Implement `HighwayMilestoneFeatureValue::Read()` reading `distance` via `ReadUInt32Number()` and string fields via `ReadString()`

- [x] 2.2 Implement `HighwayMilestoneFeatureValue::Write()` writing `distance` via `WriteNumber()` and string fields via `Write()`

- [x] 2.3 Implement `HighwayMilestoneFeatureValue::operator=` and `operator==` for all 4 fields

- [x] 2.4 Implement `HighwayMilestoneFeatureValue::GetLabel()` returning `distance` for label index 0

- [x] 2.5 Implement `HighwayMilestoneFeature::Initialize()` registering `distance`, `ref`, `carriageway_ref`, `marker` tags

- [x] 2.6 Implement `HighwayMilestoneFeature::GetName()`, `GetValueAlignment()`, `GetValueSize()`, `AllocateValue()`

- [x] 2.7 Implement `HighwayMilestoneFeature::Parse()`: only allocate value if both `distance` and `ref` tags present; parse distance via `StringToNumber()` with `.` separator, warn and skip on bad format

## 3. TypeConfig Registration

- [x] 3.1 Add `#include <osmscout/feature/HighwayMilestoneFeature.h>` to `libosmscout/src/osmscout/TypeConfig.cpp`

- [x] 3.2 Add `RegisterFeature(std::make_shared<HighwayMilestoneFeature>());` in `TypeConfig::TypeConfig()` constructor

## 4. Build System Updates

- [x] 4.1 Add `include/osmscout/feature/HighwayMilestoneFeature.h` to `HEADER_FILES_FEATURE` in `libosmscout/CMakeLists.txt`

- [x] 4.2 Add `src/osmscout/feature/HighwayMilestoneFeature.cpp` to source file list in `libosmscout/CMakeLists.txt`

- [x] 4.3 Add `osmscout/feature/HighwayMilestoneFeature.h` to header list in `libosmscout/include/meson.build`

- [x] 4.4 Add `src/osmscout/feature/HighwayMilestoneFeature.cpp` to source list in `libosmscout/src/meson.build`

## 5. Description Processor (DumpData tool)

- [x] 5.1 Add `HighwayMilestoneDescriptionProcessor` class declaration to `include/osmscout/description/DescriptionService.h`

- [x] 5.2 Add `#include <osmscout/feature/HighwayMilestoneFeature.h>` to `src/osmscout/description/DescriptionService.cpp`

- [x] 5.3 Add `HighwayMilestoneDescriptionProcessor` constants and `Process()` implementation to `src/osmscout/description/DescriptionService.cpp`

- [x] 5.4 Register `HighwayMilestoneDescriptionProcessor` in `DescriptionService::DescriptionService()` constructor
