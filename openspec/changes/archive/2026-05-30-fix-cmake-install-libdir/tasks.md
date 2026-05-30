## 1. Include GNUInstallDirs in root CMakeLists.txt

- [x] 1.1 Add `include(GNUInstallDirs)` after `project()` call (line 3) and before any subdirectory adds (est: 1)
- [x] 1.2 Use `${CMAKE_INSTALL_LIBDIR}` for config install dir in root CMakeLists.txt (line 730) (est: 1)

## 2. Fix osmscout_library_project macro (ProjectConfig.cmake)

- [x] 2.1 Replace `LIBRARY DESTINATION lib` with `LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 110) (est: 1)
- [x] 2.2 Replace `ARCHIVE DESTINATION lib` with `ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 111) (est: 1)
- [x] 2.3 Replace `FRAMEWORK DESTINATION lib` with `FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 112) (est: 1)
- [x] 2.4 Replace `RUNTIME DESTINATION bin` with `RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"` (line 108) (est: 1)
- [x] 2.5 Replace `BUNDLE DESTINATION bin` with `BUNDLE DESTINATION "${CMAKE_INSTALL_BINDIR}"` (line 109) (est: 1)
- [x] 2.6 Replace pkgconfig install `DESTINATION lib/pkgconfig` with `DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"` (line 142) (est: 1)

## 3. Fix osmscout_demo_project macro (ProjectConfig.cmake)

- [x] 3.1 Replace `LIBRARY DESTINATION lib` with `LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 177) (est: 1)
- [x] 3.2 Replace `ARCHIVE DESTINATION lib` with `ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 177) (est: 1)
- [x] 3.3 Replace `RUNTIME DESTINATION share/osmscout/demos` with `RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"` (line 177) (est: 1)

## 4. Fix osmscout_test_project macro (ProjectConfig.cmake)

- [x] 4.1 Replace `LIBRARY DESTINATION lib` with `LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 227) (est: 1)
- [x] 4.2 Replace `ARCHIVE DESTINATION lib` with `ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"` (line 227) (est: 1)
- [x] 4.3 Replace `RUNTIME DESTINATION share/osmscout/tests` with `RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"` (line 227) (est: 1)

## 5. Fix tool CMakeLists.txt files

- [x] 5.1 Fix MCPServer/CMakeLists.txt: use `${CMAKE_INSTALL_LIBDIR}` and `${CMAKE_INSTALL_BINDIR}` instead of hardcoded `lib`/`bin` (est: 1)

## 6. Verify with DESTDIR test build

- [x] 6.1 Run CMake configure + build + DESTDIR install, verify .so files land in correct directory (est: 2)
- [x] 6.2 Verify no regression on default systems where libdir=lib (est: 1)
