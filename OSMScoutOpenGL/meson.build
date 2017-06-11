osmscoutopenglSrc = [
  'src/OSMScoutOpenGL.cpp'
]

osmscoutopenglIncDir = include_directories('src')

OSMScoutOpenGL = executable('OSMScoutOpenGL',
                            osmscoutopenglSrc,
                            include_directories: [osmscoutopenglIncDir, osmscoutmapopenglIncDir, osmscoutmapIncDir, osmscoutIncDir],
                            dependencies: [mathDep, glfwDep],        
                            link_with: [osmscoutmapopengl],
                            install: true)

