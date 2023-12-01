osmscoutclientIncDir = include_directories('.')

osmscoutclientHeader = [
            'osmscoutclient/json/json.hpp',
            'osmscoutclient/json/json_fwd.hpp',
            'osmscoutclient/ClientImportExport.h',
            'osmscoutclient/DBInstance.h',
            'osmscoutclient/DBJob.h',
            'osmscoutclient/DBThread.h',
            'osmscoutclient/MapDirectory.h',
            'osmscoutclient/MapManager.h',
            'osmscoutclient/MapProvider.h',
            'osmscoutclient/OnlineTileProvider.h',
            'osmscoutclient/Settings.h',
            'osmscoutclient/VoiceProvider.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutclientHeader,
                    preserve_path: true)
else
    install_headers(osmscoutclientHeader)
endif
