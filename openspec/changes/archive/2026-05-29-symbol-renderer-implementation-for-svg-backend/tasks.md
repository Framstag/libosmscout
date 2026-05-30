## 1. Header file

- [ ] 1.1 Create SymbolRendererSVG.h with class declaration, private members, public constructor, and protected overrides
- [ ] 1.2 Add OSMSCOUT_MAP_SVG_API export macro to class declaration
- [ ] 1.3 Delete copy/move constructors and assignment operators

## 2. Implementation file

- [ ] 2.1 Implement SymbolRendererSVG constructor with default member values
- [ ] 2.2 Use Color::ToHexString() instead of duplicating hex formatting
- [ ] 2.3 Implement WriteFillAndStroke private method emitting fill/stroke SVG attributes
- [ ] 2.4 Implement SetFill reading FillStyle and setting hasFill/fillColor state
- [ ] 2.5 Implement SetBorder reading BorderStyle and setting hasStroke/strokeColor/strokeWidth state
- [ ] 2.6 Implement DrawPolygon emitting SVG polyline element with points attribute
- [ ] 2.7 Implement DrawRect emitting SVG rect element with x/y/width/height attributes
- [ ] 2.8 Implement DrawCircle emitting SVG circle element with cx/cy/r attributes

## 3. Refactor DrawSymbol

- [ ] 3.1 Replace inline primitive dispatch in MapPainterSVG::DrawSymbol with SymbolRendererSVG delegation
- [ ] 3.2 Add #include for SymbolRendererSVG.h to MapPainterSVG.cpp
- [ ] 3.3 Preserve symbol name comment output before rendering

## 4. Build system

- [ ] 4.1 Add SymbolRendererSVG.h and SymbolRendererSVG.cpp to CMakeLists.txt
- [ ] 4.2 Add SymbolRendererSVG.h to include/osmscoutmapsvg/meson.build header list
- [ ] 4.3 Add SymbolRendererSVG.cpp to src/meson.build source list

## 5. Unit test for SymbolRendererSVG

- [ ] 5.1 Create a new test source file at Tests/src/SymbolRendererSVGTest.cpp
- [ ] 5.2 Test SetFill: construct SymbolRendererSVG, call SetFill with visible FillStyle, call DrawCircle, verify output contains fill="#RRGGBB" and does not contain fill="none"
- [ ] 5.3 Test SetFill null: call SetFill with null FillStyleRef, DrawRect, verify output contains fill="none"
- [ ] 5.4 Test SetBorder: call SetBorder with borderStyle, DrawRect, verify output contains stroke="#RRGGBB" stroke-width="<w>"
- [ ] 5.5 Test SetBorder null: call SetBorder with null, DrawRect, verify output contains stroke="none"
- [ ] 5.6 Test DrawRect: verify output contains <rect x="..." y="..." width="..." height="..." />
- [ ] 5.7 Test DrawCircle: verify output contains <circle cx="..." cy="..." r="..." />
- [ ] 5.8 Test DrawPolygon: verify output contains <polyline points="x1,y1 x2,y2 ..." />
- [ ] 5.9 Test Render polygon primitive: build a Symbol with PolygonPrimitive, call Render(), verify SVG polyline with correct coordinates and fill/stroke attributes
- [ ] 5.10 Test Render rectangle primitive: build a Symbol with RectanglePrimitive, call Render(), verify SVG rect with correct attributes
- [ ] 5.11 Test Render circle primitive: build a Symbol with CirclePrimitive, call Render(), verify SVG circle with correct attributes
- [ ] 5.12 Test Render multiple primitives: build a Symbol with polygon+circle+rectangle, verify all three SVG elements appear
- [ ] 5.13 Test Render with scaleFactor: build a Symbol, Render with scaleFactor=2.0, verify coordinates are scaled
- [ ] 5.14 Test Render with projection mode MAP vs GROUND: verify different scaling behavior per projection mode
- [ ] 5.15 Register test in Tests/CMakeLists.txt with condition ${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map AND TARGET OSMScout::MapSVG

## 6. Build verification

- [ ] 6.1 Full build with SVG backend enabled compiles without errors
- [ ] 6.2 CTest passes all symbol renderer tests