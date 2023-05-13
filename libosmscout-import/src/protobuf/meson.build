if protobufDep.found() and protocCmd.found()
  protoGen = generator(protocCmd,
                       output  : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
                       arguments : ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@','@INPUT@'])

  protoSrc = protoGen.process('fileformat.proto', 'osmformat.proto')
else
  protoSrc = []
endif
