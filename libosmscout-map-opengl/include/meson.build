osmscoutmapopenglIncDir = include_directories('.')

osmscoutmapopenglHeader = [
            'osmscoutmapopengl/MapOpenGLImportExport.h',
            'osmscoutmapopengl/MapPainterOpenGL.h',
            'osmscoutmapopengl/OpenGLMapData.h',
            'osmscoutmapopengl/Triangulate.h',
            'osmscoutmapopengl/OpenGLProjection.h',
            'osmscoutmapopengl/PNGLoaderOpenGL.h',
            'osmscoutmapopengl/ShaderUtils.h',
            'osmscoutmapopengl/TextLoader.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapopenglHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapopenglHeader)
endif

