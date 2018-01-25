
# install QM files to correct location - QStandardPaths::AppDataLocation / "translations"
# Linux
translationInstallDir = 'share/libosmscout/OSMScout2/translations'

# Mac OS / iOS
if build_machine.system()=='darwin'
    translationInstallDir = '/Library/Application Support/libosmscout/OSMScout2/translations'
endif
# Windows
if build_machine.system()=='windows'
    translationInstallDir = 'C:/ProgramData/libosmscout/OSMScout2/translations'
endif

# https://github.com/mesonbuild/meson/blob/master/docs/markdown/Qt5-module.md
osmscout2Trs = qt5.compile_translations(ts_files : [
                                            'cs.ts',
                                            'en.ts',
                                        ],
                                        build_by_default : true,
                                        install: true,
                                        install_dir: translationInstallDir
                                        )
