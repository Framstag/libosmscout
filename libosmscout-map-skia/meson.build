cppArgs = []

# Detect which Skia font manager is available
cxx = meson.get_compiler('cpp')

# Try DirectWrite (Windows)
if cxx.compiles('''#include <core/SkFontMgr.h>
#include <ports/SkTypeface_win.h>
int main() {
  auto mgr = SkFontMgr_New_DirectWrite();
  return mgr ? 0 : 1;
}''',
  name: 'SkFontMgr_New_DirectWrite',
  dependencies: skiaDep)
  cppArgs += ['-DHAVE_SKIA_FONTMGR_DIRECTWRITE']
elif cxx.compiles('''#include <core/SkFontMgr.h>
#include <ports/SkTypeface_win.h>
int main() {
  auto mgr = SkFontMgr_New_GDI();
  return mgr ? 0 : 1;
}''',
  name: 'SkFontMgr_New_GDI',
  dependencies: skiaDep)
  cppArgs += ['-DHAVE_SKIA_FONTMGR_GDI']
elif cxx.compiles('''#include <core/SkFontMgr.h>
#include <ports/SkFontMgr_fontconfig.h>
#include <ports/SkFontScanner_FreeType.h>
int main() {
  auto mgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
  return mgr ? 0 : 1;
}''',
  name: 'SkFontMgr_New_FontConfig',
  dependencies: skiaDep)
  cppArgs += ['-DHAVE_SKIA_FONTMGR_FONTCONFIG']
  cppArgs += ['-DHAVE_SKIA_FONTSCANNER_FREETYPE']
elif cxx.compiles('''#include <core/SkFontMgr.h>
#include <ports/SkFontMgr_empty.h>
int main() {
  auto mgr = SkFontMgr_New_Custom_Empty();
  return mgr ? 0 : 1;
}''',
  name: 'SkFontMgr_New_Custom_Empty',
  dependencies: skiaDep)
  cppArgs += ['-DHAVE_SKIA_FONTMGR_EMPTY']
endif

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapSkia_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapskia')
subdir('include/osmscoutmapskia/private')
subdir('src')

osmscoutmapskia = library('osmscout_map_skia',
                           osmscoutmapskiaSrc,
                           include_directories: [osmscoutmapskiaIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: [mathDep, threadDep, skiaDep],
                           link_with: [osmscoutmap, osmscout],
                           version: libraryVersion,
                           install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapskia,
             name: 'libosmscout_map_skia',
             filebase: 'libosmscout_map_skia',
             description: 'Library for rendering OSM data using the Skia graphics library',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
