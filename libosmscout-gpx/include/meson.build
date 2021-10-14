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

install_headers(osmscoutgpxHeader)
