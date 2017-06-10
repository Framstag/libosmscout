/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/import/PreprocessPBF.h>

#include <osmscout/private/Config.h>
#include <osmscout/ImportFeatures.h>

#include <cstdio>

#if defined(HAVE_FCNTL_H)
  #include <fcntl.h>
#endif

// We should try to get rid of this!
#if defined(__WIN32__) || defined(WIN32)
  #include <winsock.h>
#else
  #include <arpa/inet.h>
#endif

#if defined(HAVE_LIB_ZLIB) || defined(OSMSCOUT_IMPORT_HAVE_PROTOBUF_SUPPORT)
  #include <zlib.h>
#endif

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

#define MAX_BLOCK_HEADER_SIZE (64*1024)
#define MAX_BLOB_SIZE         (32*1024*1024)
#define NANO                  (1000.0*1000.0*1000.0)

namespace osmscout {

  bool PreprocessPBF::GetPos(FILE* file,
                             FileOffset& pos) const
  {
#if defined(__WIN32__) || defined(WIN32)
    const __int64 filepos=_ftelli64(file);

    if (filepos==-1) {
      return false;
    }
    else {
      pos=(FileOffset)filepos;
    }
#elif defined(HAVE_FSEEKO)
    off_t filepos=ftello(file);

    if (filepos==-1) {
      return false;
    }
    else {
      pos=(FileOffset)filepos;
    }
#else
    long filepos=ftell(file);

    if (filepos==-1) {
      return false;
    }
    else {
      pos=(FileOffset)filepos;
    }
#endif

    return true;
  }

  void PreprocessPBF::AssureBlockSize(google::protobuf::int32 length)
  {
    if (buffer==NULL) {
      buffer=new char[length];
      bufferSize=length;
    }
    else if (bufferSize<length) {
      delete buffer;
      buffer=new char[length];
      bufferSize=length;
    }
  }

  bool PreprocessPBF::ReadBlockHeader(Progress& progress,
                                      FILE* file,
                                      OSMPBF::BlobHeader& blockHeader,
                                      bool silent)
  {
    uint32_t blockHeaderLength;

    if (fread(&blockHeaderLength,4,1,file)!=1) {
      if (!silent) {
        progress.Error("Cannot read block header length!");
      }
      return false;
    }

    uint32_t length=ntohl(blockHeaderLength);

    if (length==0 || length>MAX_BLOCK_HEADER_SIZE) {
      progress.Error("Block header size invalid!");
      return false;
    }

    AssureBlockSize(length);

    if (fread(buffer,sizeof(char),length,file)!=length) {
      progress.Error("Cannot read block header!");
      delete[] buffer;
      return false;
    }

    if (!blockHeader.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse block header!");
      return false;
    }

    return true;
  }

  bool PreprocessPBF::ReadHeaderBlock(Progress& progress,
                                      FILE* file,
                                      const OSMPBF::BlobHeader& blockHeader,
                                      OSMPBF::HeaderBlock& headerBlock)
  {
    OSMPBF::Blob            blob;
    google::protobuf::int32 length=blockHeader.datasize();

    if (length==0 || length>MAX_BLOB_SIZE) {
      progress.Error("Blob size invalid!");
      return false;
    }

    AssureBlockSize(length);

    if (fread(buffer,sizeof(char),length,file)!=(size_t)length) {
      progress.Error("Cannot read blob!");
      return false;
    }

    if (!blob.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse blob!");
      return false;
    }

    if (blob.has_raw()) {
      length=(uint32_t)blob.raw().length();
      AssureBlockSize(length);
      memcpy(buffer,blob.raw().data(),(size_t)length);
    }
    else if (blob.has_zlib_data()) {
#if defined(HAVE_LIB_ZLIB) || defined(OSMSCOUT_IMPORT_HAVE_PROTOBUF_SUPPORT)
      length=blob.raw_size();
      AssureBlockSize(length);

      z_stream compressedStream;

      compressedStream.next_in=(Bytef*)const_cast<char*>(blob.zlib_data().data());
      compressedStream.avail_in=(uint32_t)blob.zlib_data().size();
      compressedStream.next_out=(Bytef*)buffer;
      compressedStream.avail_out=(uInt)length;
      compressedStream.zalloc=Z_NULL;
      compressedStream.zfree=Z_NULL;
      compressedStream.opaque=Z_NULL;

      if (inflateInit( &compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        return false;
      }

      if (inflate(&compressedStream,Z_FINISH)!=Z_STREAM_END) {
        progress.Error("Cannot decode zlib compressed blob data!");
        return false;
      }

      if (inflateEnd(&compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        return false;
      }
#else
      progress.Error("Data is zlib encoded but zlib support is not enabled!");
      return false;
#endif
    }
    else if (blob.has_lzma_data()) {
      progress.Error("Data is lzma encoded but lzma support is not enabled!");
      return false;
    }

    if (!headerBlock.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse header block!");
      return false;
    }

    return true;
  }

  bool PreprocessPBF::ReadPrimitiveBlock(Progress& progress,
                                         FILE* file,
                                         const OSMPBF::BlobHeader& blockHeader,
                                         OSMPBF::PrimitiveBlock& primitiveBlock)
  {
    OSMPBF::Blob            blob;
    google::protobuf::int32 length=blockHeader.datasize();

    if (length==0 || length>MAX_BLOB_SIZE) {
      progress.Error("Blob size invalid!");
      return false;
    }

    AssureBlockSize(length);

    if (fread(buffer,sizeof(char),length,file)!=(size_t)length) {
      progress.Error("Cannot read blob!");
      return false;
    }

    if (!blob.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse blob!");
      return false;
    }

    if (blob.has_raw()) {
      length=(uint32_t)blob.raw().length();
      AssureBlockSize(length);
      memcpy(buffer,blob.raw().data(),(size_t)length);
    }
    else if (blob.has_zlib_data()) {
#if defined(HAVE_LIB_ZLIB) || defined(OSMSCOUT_IMPORT_HAVE_PROTOBUF_SUPPORT)
      length=blob.raw_size();
      AssureBlockSize(length);

      z_stream compressedStream;

      compressedStream.next_in=(Bytef*)const_cast<char*>(blob.zlib_data().data());
      compressedStream.avail_in=(uint32_t)blob.zlib_data().size();
      compressedStream.next_out=(Bytef*)buffer;
      compressedStream.avail_out=(uInt)length;
      compressedStream.zalloc=Z_NULL;
      compressedStream.zfree=Z_NULL;
      compressedStream.opaque=Z_NULL;

      if (inflateInit( &compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        return false;
      }

      if (inflate(&compressedStream,Z_FINISH)!=Z_STREAM_END) {
        progress.Error("Cannot decode zlib compressed blob data!");
        return false;
      }

      if (inflateEnd(&compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        return false;
      }
#else
      progress.Error("Data is zlib encoded but zlib support is not enabled!");
      return false;
#endif
    }
    else if (blob.has_lzma_data()) {
      progress.Error("Data is lzma encoded but lzma support is not enabled!");
      return false;
    }

    if (!primitiveBlock.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse primitive block!");
      return false;
    }

    return true;
  }

  void PreprocessPBF::ReadNodes(const TypeConfig& typeConfig,
                                const OSMPBF::PrimitiveBlock& block,
                                const OSMPBF::PrimitiveGroup& group,
                                PreprocessorCallback::RawBlockData& data)
  {
    data.nodeData.reserve(data.nodeData.size()+group.nodes_size());

    for (int n=0; n<group.nodes_size(); n++) {
      PreprocessorCallback::RawNodeData nodeData;

      const OSMPBF::Node &inputNode=group.nodes(n);

      nodeData.id=inputNode.id();
      nodeData.coord.Set((inputNode.lat()*block.granularity()+block.lat_offset())/NANO,
                         (inputNode.lon()*block.granularity()+block.lon_offset())/NANO);

      tagMap.clear();

      for (int t=0; t<inputNode.keys_size(); t++) {
        TagId id=typeConfig.GetTagId(block.stringtable().s(inputNode.keys(t)));

        if (id!=tagIgnore) {
          nodeData.tags[id]=block.stringtable().s(inputNode.vals(t));
        }
      }

      data.nodeData.push_back(std::move(nodeData));
    }
  }

  void PreprocessPBF::ReadDenseNodes(const TypeConfig& typeConfig,
                                     const OSMPBF::PrimitiveBlock& block,
                                     const OSMPBF::PrimitiveGroup& group,
                                     PreprocessorCallback::RawBlockData& data)
  {
    const OSMPBF::DenseNodes& dense=group.dense();
    Id                        dId=0;
    double                    dLat=0;
    double                    dLon=0;
    int                       t=0;

    data.nodeData.reserve(data.nodeData.size()+dense.id_size());

    for (int d=0; d<dense.id_size();d++) {
      PreprocessorCallback::RawNodeData nodeData;

      dId+=dense.id(d);
      dLat+=dense.lat(d);
      dLon+=dense.lon(d);

      nodeData.id=dId;
      nodeData.coord.Set((dLat*block.granularity()+block.lat_offset())/NANO,
                     (dLon*block.granularity()+block.lon_offset())/NANO);

      while (true) {
        if (t>=dense.keys_vals_size()) {
          break;
        }

        if (dense.keys_vals(t)==0) {
          t++;
          break;
        }

        TagId id=typeConfig.GetTagId(block.stringtable().s(dense.keys_vals(t)));

        if (id!=tagIgnore) {
          nodeData.tags[id]=block.stringtable().s(dense.keys_vals(t+1));
        }

        t+=2;
      }

      data.nodeData.push_back(std::move(nodeData));
    }
  }

  void PreprocessPBF::ReadWays(const TypeConfig& typeConfig,
                               const OSMPBF::PrimitiveBlock& block,
                               const OSMPBF::PrimitiveGroup& group,
                               PreprocessorCallback::RawBlockData& data)
  {
    data.wayData.reserve(data.wayData.size()+group.ways_size());

    for (int w=0; w<group.ways_size(); w++) {
      PreprocessorCallback::RawWayData wayData;

      const OSMPBF::Way &inputWay=group.ways(w);

      wayData.id=inputWay.id();

      for (int t=0; t<inputWay.keys_size(); t++) {
        TagId id=typeConfig.GetTagId(block.stringtable().s(inputWay.keys(t)));

        if (id!=tagIgnore) {
          wayData.tags[id]=block.stringtable().s(inputWay.vals(t));
        }
      }

      wayData.nodes.reserve((size_t)inputWay.refs_size());

      OSMId ref=0;
      for (int r=0; r<inputWay.refs_size(); r++) {
        ref+=inputWay.refs(r);

        wayData.nodes.push_back(ref);
      }

      data.wayData.push_back(std::move(wayData));
    }
  }

  void PreprocessPBF::ReadRelations(const TypeConfig& typeConfig,
                                    const OSMPBF::PrimitiveBlock& block,
                                    const OSMPBF::PrimitiveGroup& group,
                                    PreprocessorCallback::RawBlockData& data)
  {
    data.relationData.reserve(data.relationData.size()+group.relations_size());

    for (int r=0; r<group.relations_size(); r++) {
      PreprocessorCallback::RawRelationData relationData;

      const OSMPBF::Relation &inputRelation=group.relations(r);

      relationData.id=inputRelation.id();

      members.clear();

      for (int t=0; t<inputRelation.keys_size(); t++) {
        TagId id=typeConfig.GetTagId(block.stringtable().s(inputRelation.keys(t)));

        if (id!=tagIgnore) {
          relationData.tags[id]=block.stringtable().s(inputRelation.vals(t));
        }
      }

      relationData.members.reserve((size_t)inputRelation.types_size());

      Id ref=0;
      for (int m=0; m<inputRelation.types_size(); m++) {
        RawRelation::Member member;

        switch (inputRelation.types(m)) {
        case OSMPBF::Relation::NODE:
          member.type=RawRelation::memberNode;
          break;
        case OSMPBF::Relation::WAY:
          member.type=RawRelation::memberWay;
          break;
        case OSMPBF::Relation::RELATION:
          member.type=RawRelation::memberRelation;
          break;
        }

        ref+=inputRelation.memids(m);

        member.id=ref;
        member.role=block.stringtable().s(inputRelation.roles_sid(m));

        relationData.members.push_back(member);
      }

      data.relationData.push_back(std::move(relationData));
    }
  }

  PreprocessPBF::PreprocessPBF(PreprocessorCallback& callback)
  : buffer(NULL),
    bufferSize(0),
    callback(callback)
  {
    // no code
  }

  PreprocessPBF::~PreprocessPBF()
  {
    delete buffer;
  }

  void PreprocessPBF::ProcessBlock(const TypeConfigRef& typeConfig,
                                   std::unique_ptr<OSMPBF::PrimitiveBlock>&& block)
  {
    PreprocessorCallback::RawBlockDataRef blockData(new PreprocessorCallback::RawBlockData());

    for (int currentGroup=0;
         currentGroup<block->primitivegroup_size();
         currentGroup++) {
      const OSMPBF::PrimitiveGroup &group=block->primitivegroup(currentGroup);

      if (group.nodes_size()>0) {
        ReadNodes(*typeConfig,
                  *block,
                  group,
                  *blockData);
      }
      else if (group.has_dense()) {
        ReadDenseNodes(*typeConfig,
                       *block,
                       group,
                       *blockData);
      }
      else if (group.ways_size()>0) {
        ReadWays(*typeConfig,
                 *block,
                 group,
                 *blockData);
      }
      else if (group.relations_size()>0) {
        ReadRelations(*typeConfig,
                      *block,
                      group,
                      *blockData);
      }
    }

    callback.ProcessBlock(std::move(blockData));
  }

  bool PreprocessPBF::Import(const TypeConfigRef& typeConfig,
                             const ImportParameter& /*parameter*/,
                             Progress& progress,
                             const std::string& filename)
  {
    FileOffset fileSize;
    FileOffset currentPosition;

    progress.SetAction(std::string("Parsing *.osm.pbf file '")+filename+"'");

    try {
      fileSize=GetFileSize(filename);

      FILE* file;

      file=fopen(filename.c_str(),"rb");

      if (file==NULL) {
        progress.Error("Cannot open file!");
        return false;
      }

      // BlockHeader

      OSMPBF::BlobHeader blockHeader;

      if (!ReadBlockHeader(progress,file,blockHeader,false)) {
        fclose(file);
        return false;
      }

      if (blockHeader.type()!="OSMHeader") {
        progress.Error("File '"+filename+"' is not an OSM PBF file!");
        fclose(file);
        return false;
      }

      OSMPBF::HeaderBlock headerBlock;

      if (!ReadHeaderBlock(progress,
                           file,
                           blockHeader,
                           headerBlock)) {
        fclose(file);
        return false;
      }

      for (int i=0; i<headerBlock.required_features_size(); i++) {
        std::string feature=headerBlock.required_features(i);
        if (feature!="OsmSchema-V0.6" &&
            feature!="DenseNodes") {
          progress.Error(std::string("Unsupported feature '")+feature+"'");
          fclose(file);
          return false;
        }
      }

      nodes.reserve(20000);
      members.reserve(2000);

      std::future<void> currentBlockTask;

      while (true) {
        OSMPBF::BlobHeader blockHeader;

        if (!GetPos(file,
                    currentPosition)) {
          progress.Error("Cannot read current position in '"+filename+"'!");
          fclose(file);
          return false;
        }

        progress.SetProgress(currentPosition,
                             fileSize);

        if (!ReadBlockHeader(progress,
                             file,
                             blockHeader,
                             true)) {
          fclose(file);
          break;
        }

        if (blockHeader.type()!="OSMData") {
          progress.Error("File '"+filename+"' is not an OSM PBF file!");
          fclose(file);
          return false;
        }

        std::unique_ptr<OSMPBF::PrimitiveBlock> block(new OSMPBF::PrimitiveBlock());

        if (!ReadPrimitiveBlock(progress,
                                file,
                                blockHeader,
                                *block)) {
          fclose(file);
          return false;
        }

        if (currentBlockTask.valid()) {
          currentBlockTask.get();
        }

        currentBlockTask=std::async(std::launch::async,
                                    &PreprocessPBF::ProcessBlock,this,
                                    typeConfig,
                                    std::move(block));
      }

      if (currentBlockTask.valid()) {
        currentBlockTask.get();
      }
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }
}

