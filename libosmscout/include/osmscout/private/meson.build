coreCfg = configuration_data()
coreCfg.set('HAVE_VISIBILITY',haveVisibility, description: 'compiler supports simple visibility declarations')
coreCfg.set('HAVE_FCNTL_H',fcntlAvailable, description: '<fcntl.h> is available')
coreCfg.set('HAVE_CODECVT',codecvtAvailable, description: '<codecvt> is available')
coreCfg.set('HAVE_SYS_STAT_H',statAvailable, description: '<sys/stat.h> header available')
coreCfg.set('HAVE_FSEEKO',fseekoAvailable, description: 'fseeko() is available')
coreCfg.set('HAVE__FSEEKI64',fseeki64Available, description: '_fseeki64() is available')
coreCfg.set('HAVE__FTELLI64',ftelli64Available, description: '_ftelli64() is available')
coreCfg.set('HAVE_MMAP',mmapAvailable, description: 'mmap() is available')
coreCfg.set('HAVE_POSIX_FADVISE',posixfadviceAvailable, description: 'posixfadvice() is available')
coreCfg.set('HAVE_POSIX_MADVISE',posixmadviceAvailable, description: 'posixmadvice() is available')
coreCfg.set('SIZEOF_WCHAR_T',sizeOfWChar, description: 'byte size of wchar_t')
coreCfg.set('HAVE_ICONV',iconvAvailable, description: 'iconv library available')

## TODO
coreCfg.set('ICONV_CONST','', description: 'Signature of scond parameter of the iconv() function')

configure_file(output: 'Config.h',
               configuration: coreCfg)


