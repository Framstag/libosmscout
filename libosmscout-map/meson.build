cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMap_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmap')
subdir('include/osmscoutmap/private')
subdir('src')

osmscoutmapDeps = [mathDep, threadDep]

# The family name of a configured font file is read with FreeType where it is
# available; without it a font name is passed on unchanged
if ftDep.found()
  osmscoutmapDeps += [ftDep]
endif

osmscoutmap = library('osmscout_map',
                      osmscoutmapSrc,
                      include_directories: [osmscoutmapIncDir, osmscoutIncDir],
                      cpp_args: cppArgs,
                      dependencies: osmscoutmapDeps,
                      link_with: [osmscout],
                      version: libraryVersion,
                      install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmap,
             name: 'libosmscout_map',
             filebase: 'libosmscout_map',
             description: 'Base library for rendering OSM data via libosmscout databases',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
