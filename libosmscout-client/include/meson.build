osmscoutclientIncDir = include_directories('.')

osmscoutclientHeader = [
            'osmscoutclient/ClientImportExport.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutclientHeader,
                    preserve_path: true)
else
    install_headers(osmscoutclientHeader)
endif
