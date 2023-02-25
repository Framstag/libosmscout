osmscoutgpxIncDir = include_directories('.')

osmscoutgpxHeader = [
            'osmscoutgpx/GPXImportExport.h',
            'osmscoutgpx/GpxFile.h',
            'osmscoutgpx/Utils.h',
            'osmscoutgpx/Route.h',
            'osmscoutgpx/Track.h',
            'osmscoutgpx/Waypoint.h',
            'osmscoutgpx/TrackPoint.h',
            'osmscoutgpx/TrackSegment.h',
            'osmscoutgpx/Extensions.h',
          ]

if xml2Dep.found()
  osmscoutgpxHeader += [
            'osmscoutgpx/Import.h',
            'osmscoutgpx/Export.h',
          ]
endif

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutgpxHeader,
                    preserve_path: true)
else
    install_headers(osmscoutgpxHeader)
endif

