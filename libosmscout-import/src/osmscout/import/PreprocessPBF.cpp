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

#include <osmscout/private/Config.h>

#include <osmscout/import/PreprocessPBF.h>

#include <cstdio>

// We should try to get rid of this!
#if defined(__WIN32__) || defined(WIN32)
  #include <winsock.h>
#else
  #include <arpa/inet.h>
#endif

#if defined(HAVE_LIB_ZLIB)
  #include <zlib.h>
#endif

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

#include "osmscout/import/Preprocess.h"

#define MAX_BLOCK_HEADER_SIZE (64*1024)
#define MAX_BLOB_SIZE         (32*1024*1024)
#define NANO                  (1000.0*1000.0*1000.0)

namespace osmscout {

  bool ReadBlockHeader(Progress& progress,
                       FILE* file,
                       PBF::BlockHeader& blockHeader,
                       bool silent)
  {
    char blockHeaderLength[4];

    if (fread(blockHeaderLength,sizeof(char),4,file)!=4) {
      if (!silent) {
        progress.Error("Cannot read block header length!");
      }
      return false;
    }

    // ugly!
    uint32_t length=ntohl(*((uint32_t*)&blockHeaderLength));

    if (length==0 || length>MAX_BLOCK_HEADER_SIZE) {
      progress.Error("Block header size invalid!");
      return false;
    }

    char *buffer=new char[length];

    if (fread(buffer,sizeof(char),length,file)!=length) {
      progress.Error("Cannot read block header!");
      delete[] buffer;
      return false;
    }

    if (!blockHeader.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse block header!");
      delete[] buffer;
      return false;
    }

    delete[] buffer;

    return true;
  }

  bool ReadHeaderBlock(Progress& progress,
                       FILE* file,
                       const PBF::BlockHeader& blockHeader,
                       PBF::HeaderBlock& headerBlock)
  {
    PBF::Blob blob;

    uint32_t length = blockHeader.datasize();

    if (length==0 || length>MAX_BLOB_SIZE) {
      progress.Error("Blob size invalid!");
      return false;
    }

    char *buffer=new char[length];

    if (fread(buffer,sizeof(char),length,file)!=length) {
      progress.Error("Cannot read blob!");
      delete[] buffer;
      return false;
    }

    if (!blob.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse blob!");
      delete[] buffer;
      return false;
    }

    delete [] buffer;

    if (blob.has_raw()) {
      length=(uint32_t)blob.raw().length();
      buffer = new char[length];
      memcpy(buffer,blob.raw().data(),length);
    }
    else if (blob.has_zlib_data()){
#if defined(HAVE_LIB_ZLIB)
      length=blob.raw_size();
      buffer=new char[length];

      z_stream compressedStream;

      compressedStream.next_in=(Bytef*)const_cast<char*>(blob.zlib_data().data());
      compressedStream.avail_in=(uint32_t)blob.zlib_data().size();
      compressedStream.next_out=(Bytef*)buffer;
      compressedStream.avail_out=length;
      compressedStream.zalloc=Z_NULL;
      compressedStream.zfree=Z_NULL;
      compressedStream.opaque=Z_NULL;

      if (inflateInit( &compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        delete[] buffer;
        return false;
      }

      if (inflate(&compressedStream,Z_FINISH)!=Z_STREAM_END) {
        progress.Error("Cannot decode zlib compressed blob data!");
        delete[] buffer;
        return false;
      }

      if (inflateEnd(&compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        delete[] buffer;
        return false;
      }
#else
      progress.Error("Data is zlib encoded but zlib support is not enabled!");
      return false;
#endif
    }
    else if (blob.has_bzip2_data()){
      progress.Error("Data is bzip2 encoded but bzip2 support is not enabled!");
      return false;
    }
    else if (blob.has_lzma_data()){
      progress.Error("Data is lzma encoded but lzma support is not enabled!");
      return false;
    }

    if (!headerBlock.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse header block!");
      delete[] buffer;
      return false;
    }

    delete[] buffer;

    return true;
  }

  bool ReadPrimitiveBlock(Progress& progress,
                          FILE* file,
                          const PBF::BlockHeader& blockHeader,
                          PBF::PrimitiveBlock& primitiveBlock)
  {
    PBF::Blob blob;

    uint32_t length = blockHeader.datasize();

    if (length==0 || length>MAX_BLOB_SIZE) {
      progress.Error("Blob size invalid!");
      return false;
    }

    char *buffer=new char[length];

    if (fread(buffer,sizeof(char),length,file)!=length) {
      progress.Error("Cannot read blob!");
      delete[] buffer;
      return false;
    }

    if (!blob.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse blob!");
      delete[] buffer;
      return false;
    }

    delete [] buffer;

    if (blob.has_raw()) {
      length=(uint32_t)blob.raw().length();
      buffer = new char[length];
      memcpy(buffer,blob.raw().data(),length);
    }
    else if (blob.has_zlib_data()){
#if defined(HAVE_LIB_ZLIB)
      length=blob.raw_size();
      buffer=new char[length];

      z_stream compressedStream;

      compressedStream.next_in=(Bytef*)const_cast<char*>(blob.zlib_data().data());
      compressedStream.avail_in=(uint32_t)blob.zlib_data().size();
      compressedStream.next_out=(Bytef*)buffer;
      compressedStream.avail_out=length;
      compressedStream.zalloc=Z_NULL;
      compressedStream.zfree=Z_NULL;
      compressedStream.opaque=Z_NULL;

      if (inflateInit( &compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        delete[] buffer;
        return false;
      }

      if (inflate(&compressedStream,Z_FINISH)!=Z_STREAM_END) {
        progress.Error("Cannot decode zlib compressed blob data!");
        delete[] buffer;
        return false;
      }

      if (inflateEnd(&compressedStream)!=Z_OK) {
        progress.Error("Cannot decode zlib compressed blob data!");
        delete[] buffer;
        return false;
      }
#else
      progress.Error("Data is zlib encoded but zlib support is not enabled!");
      return false;
#endif
    }
    else if (blob.has_bzip2_data()){
      progress.Error("Data is bzip2 encoded but bzip2 support is not enabled!");
      return false;
    }
    else if (blob.has_lzma_data()){
      progress.Error("Data is lzma encoded but lzma support is not enabled!");
      return false;
    }

    if (!primitiveBlock.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse primitive block!");
      delete[] buffer;
      return false;
    }

    delete[] buffer;

    return true;
  }

  void PreprocessPBF::ReadNodes(const TypeConfig& typeConfig,
                                const PBF::PrimitiveBlock& block,
                                const PBF::PrimitiveGroup& group)
  {
    for (int n=0; n<group.nodes_size(); n++) {
      const PBF::Node &inputNode=group.nodes(n);

      tagMap.clear();

      for (int t=0; t<inputNode.keys_size(); t++) {
        TagId id=typeConfig.GetTagId(block.stringtable().s(inputNode.keys(t)).c_str());

        if (id!=tagIgnore) {
          tagMap[id]=block.stringtable().s(inputNode.vals(t));
        }
      }

      callback.ProcessNode(inputNode.id(),
                           (inputNode.lon()*block.granularity()+block.lon_offset())/NANO,
                           (inputNode.lat()*block.granularity()+block.lat_offset())/NANO,
                           tagMap);
    }
  }

  void PreprocessPBF::ReadDenseNodes(const TypeConfig& typeConfig,
                                     const PBF::PrimitiveBlock& block,
                                     const PBF::PrimitiveGroup& group)
  {
    const PBF::DenseNodes &dense=group.dense();
    Id     dId=0;
    double dLat=0;
    double dLon=0;
    int    t=0;

    for (int d=0; d<dense.id_size();d++) {
      dId+=dense.id(d);
      dLat+=dense.lat(d);
      dLon+=dense.lon(d);

      tagMap.clear();

      while (true) {
        if (t>=dense.keys_vals_size()) {
          break;
        }

        if (dense.keys_vals(t)==0) {
          t++;
          break;
        }

        TagId id=typeConfig.GetTagId(block.stringtable().s(dense.keys_vals(t)).c_str());

        if (id!=tagIgnore) {
          tagMap[id]=block.stringtable().s(dense.keys_vals(t+1));
        }

        t+=2;
      }

      callback.ProcessNode(dId,
                           (dLon*block.granularity()+block.lon_offset())/NANO,
                           (dLat*block.granularity()+block.lat_offset())/NANO,
                           tagMap);
    }
  }

  void PreprocessPBF::ReadWays(const TypeConfig& typeConfig,
                               const PBF::PrimitiveBlock& block,
                               const PBF::PrimitiveGroup& group)
  {
    for (int w=0; w<group.ways_size(); w++) {
      const PBF::Way &inputWay=group.ways(w);

      nodes.clear();
      tagMap.clear();

      for (int t=0; t<inputWay.keys_size(); t++) {
        TagId id=typeConfig.GetTagId(block.stringtable().s(inputWay.keys(t)).c_str());

        if (id!=tagIgnore) {
          tagMap[id]=block.stringtable().s(inputWay.vals(t));
        }
      }

      unsigned long ref=0;
      for (int r=0; r<inputWay.refs_size(); r++) {
        ref+=inputWay.refs(r);

        nodes.push_back(ref);
      }

      callback.ProcessWay(inputWay.id(),
                          nodes,
                          tagMap);
    }
  }

  void PreprocessPBF::ReadRelations(const TypeConfig& typeConfig,
                                    const PBF::PrimitiveBlock& block,
                                    const PBF::PrimitiveGroup& group)
  {
    for (int r=0; r<group.relations_size(); r++) {
      const PBF::Relation &inputRelation=group.relations(r);

      members.clear();
      tagMap.clear();

      for (int t=0; t<inputRelation.keys_size(); t++) {
        TagId id=typeConfig.GetTagId(block.stringtable().s(inputRelation.keys(t)).c_str());

        if (id!=tagIgnore) {
          tagMap[id]=block.stringtable().s(inputRelation.vals(t));
        }
      }

      Id ref=0;
      for (int r=0; r<inputRelation.types_size();r++) {
        RawRelation::Member member;

        switch (inputRelation.types(r)) {
        case PBF::Relation::NODE:
          member.type=RawRelation::memberNode;
          break;
        case PBF::Relation::WAY:
          member.type=RawRelation::memberWay;
          break;
        case PBF::Relation::RELATION:
          member.type=RawRelation::memberRelation;
          break;
        }

        ref+=inputRelation.memids(r);

        member.id=ref;
        member.role=block.stringtable().s(inputRelation.roles_sid(r));

        members.push_back(member);
      }

      callback.ProcessRelation(inputRelation.id(),
                               members,
                               tagMap);
    }
  }

  PreprocessPBF::PreprocessPBF(PreprocessorCallback& callback)
  : callback(callback)
  {
    // no code
  }

  bool PreprocessPBF::Import(const TypeConfigRef& typeConfig,
                             const ImportParameter& /*parameter*/,
                             Progress& progress,
                             const std::string& filename)
  {
    progress.SetAction(std::string("Parsing *.osm.pbf file '")+filename+"'");

    FILE* file;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      progress.Error("Cannot open file!");
      return false;
    }

    // BlockHeader

    PBF::BlockHeader blockHeader;

    if (!ReadBlockHeader(progress,file,blockHeader,false)) {
      fclose(file);
      return false;
    }

    if (blockHeader.type()!="OSMHeader") {
      progress.Error("File is not an OSM PBF file!");
      fclose(file);
      return false;
    }

    PBF::HeaderBlock headerBlock;

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

    while (true) {
      PBF::BlockHeader blockHeader;

      if (!ReadBlockHeader(progress,
                           file,
                           blockHeader,
                           true)) {
        fclose(file);
        break;
      }

      if (blockHeader.type()!="OSMData") {
        progress.Error("File is not an OSM PBF file!");
        fclose(file);
        return false;
      }

      PBF::PrimitiveBlock block;

      if (!ReadPrimitiveBlock(progress,
                              file,
                              blockHeader,
                              block)) {
        fclose(file);
        return false;
      }

      for (int currentGroup=0;
           currentGroup<block.primitivegroup_size();
           currentGroup++) {
        const PBF::PrimitiveGroup &group=block.primitivegroup(currentGroup);

        if (group.nodes_size()>0) {
          ReadNodes(*typeConfig,
                    block,
                    group);
        }
        else if (group.ways_size()>0) {
          ReadWays(*typeConfig,
                   block,
                   group);
        }
        else if (group.relations_size()>0) {
          ReadRelations(*typeConfig,
                        block,
                        group);
        }
        else if (group.has_dense()) {
          ReadDenseNodes(*typeConfig,
                         block,
                         group);
        }
      }
    }

    return true;
  }
}

