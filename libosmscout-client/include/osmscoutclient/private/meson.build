clientCfg = configuration_data()
clientCfg.set('HAVE_VISIBILITY',haveVisibility, description: 'compiler supports simple visibility declarations')
clientCfg.set('HAVE_STD_EXECUTION',stdExecutionAvailable, description: 'compiler supports C++17 execution')

configure_file(output: 'Config.h',
               configuration: clientCfg)

