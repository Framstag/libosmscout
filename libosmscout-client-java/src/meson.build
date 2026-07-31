cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutClientJava_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

link_args = []
if compiler.get_id()=='gcc' and compiler.version().version_compare('<=8.3.0')
  # GCC <= 8.3 needs explicit filesystem library linking
  link_args += '-lstdc++fs'
endif
osmscoutclientjavaIncDir = include_directories('../include')

osmscoutclientjavaSrc = [
  'OSMScoutClient.cpp'
]

gpxDeps = []
gpxLink = []
gpxIncludes = []
if buildGpx
  gpxDeps = [xml2Dep]
  gpxLink = [osmscoutgpx]
  gpxIncludes = [osmscoutgpxIncDir]
endif

cairoDeps = []
cairoLink = []
cairoIncludes = []
if buildMapCairo
  cairoDeps = [cairoDep, pangoDep, pangocairoDep]
  cairoLink = [osmscoutmapcairo]
  cairoIncludes = [osmscoutmapcairoIncDir]
  cppArgs += ['-DHAVE_MAP_CAIRO=1']
endif

osmscoutclientjava = library('osmscout_client_java',
                         [osmscoutclientjavaSrc, osmscoutclientjavaJniHeaders],
                         include_directories: [osmscoutclientjavaIncDir, osmscoutIncDir, osmscoutmapIncDir, osmscoutclientIncDir, osmscoutclientjavaJniIncDir, cairoIncludes, gpxIncludes],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, jniDep, cairoDeps, gpxDeps],
                         link_args: link_args,
                         link_with: [osmscout, osmscoutmap, osmscoutclient, cairoLink, gpxLink],
                         version: libraryVersion,
                         install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutclientjava,
             name: 'libosmscout_client_java',
             filebase: 'libosmscout_client_java',
             description: 'Library for creating libosmscout Java clients',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
