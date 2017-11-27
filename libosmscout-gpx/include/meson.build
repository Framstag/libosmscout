osmscoutgpxIncDir = include_directories('.')

osmscoutgpxHeader = [
            'osmscout/private/GPXImportExport.h',
            'osmscout/gpx/Optional.h',
            'osmscout/gpx/GpxFile.h',
            'osmscout/gpx/Utils.h',
            'osmscout/gpx/Route.h',
            'osmscout/gpx/Track.h',
            'osmscout/gpx/Waypoint.h',
            'osmscout/gpx/TrackPoint.h',
            'osmscout/gpx/TrackSegment.h',
          ]

if xml2Dep.found()
  osmscoutgpxHeader += [
            'osmscout/gpx/Import.h',
            'osmscout/gpx/Export.h',
          ]
endif

install_headers(osmscoutgpxHeader)
