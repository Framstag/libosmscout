MAINTAINERCLEANFILES = Makefile.in

PROTOC = @protoc@
PROTOC_FLAGS = \
	--cpp_out=$(top_srcdir)/src/protobuf \
	--proto_path=$(top_srcdir)/src/protobuf

filesdir = $(pkgdatadir)/protobuf

if HAVE_PROG_PROTOC
files_DATA = fileformat.pb.cc osmformat.pb.cc
endif

EXTRA_DIST = fileformat.proto \
	      osmformat.proto

fileformat.pb.cc: Makefile fileformat.proto
	$(PROTOC) $(PROTOC_FLAGS) \
	$(top_srcdir)/src/protobuf/fileformat.proto
	cp $(top_srcdir)/src/protobuf/fileformat.pb.cc $(top_srcdir)/src/osmscout/import/pbf
	cp $(top_srcdir)/src/protobuf/fileformat.pb.h $(top_srcdir)/include/osmscout/import/pbf

osmformat.pb.cc: Makefile osmformat.proto
	$(PROTOC) $(PROTOC_FLAGS) \
	$(top_srcdir)/src/protobuf/osmformat.proto
	cp $(top_srcdir)/src/protobuf/osmformat.pb.cc $(top_srcdir)/src/osmscout/import/pbf
	cp $(top_srcdir)/src/protobuf/osmformat.pb.h $(top_srcdir)/include/osmscout/import/pbf

clean-local:
	rm -f *.cc *.h
	rm -f $(top_srcdir)/src/osmscout/import/pbf/*.cc
	rm -f $(top_srcdir)/include/osmscout/import/pbf/*.h
