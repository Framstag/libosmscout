if protobufDep.found() and protocCmd.found()
  protoGen = generator(protocCmd,
                       output  : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
                       arguments : ['--cpp_out=@BUILD_DIR@','--proto_path=@SOURCE_DIR@/libosmscout-import/src/protobuf','@INPUT@'])

  protoSrc = protoGen.process('fileformat.proto', 'osmformat.proto')
else
  protoSrc = []
endif

