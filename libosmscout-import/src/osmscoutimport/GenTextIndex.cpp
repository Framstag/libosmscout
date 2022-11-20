/*
 This source is part of the libosmscout library
 Copyright (C) 2013 Preet Desai

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

#include <osmscout/ObjectRef.h>

#include <osmscout/Node.h>
#include <osmscout/Way.h>
#include <osmscout/Area.h>

#include <osmscout/TypeFeatures.h>
#include <osmscout/FeatureReader.h>

#include <osmscout/TextSearchIndex.h>

#include <osmscout/NodeDataFile.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/AreaDataFile.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

#include <osmscoutimport/GenTextIndex.h>

#include <marisa.h>

namespace osmscout
{
  void TextIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                          ImportModuleDescription& description) const
  {
    description.SetName("TextIndexGenerator");
    description.SetDescription("Generate text based object search");

    description.AddRequiredFile(NodeDataFile::NODES_DAT);
    description.AddRequiredFile(WayDataFile::WAYS_DAT);
    description.AddRequiredFile(AreaDataFile::AREAS_DAT);

    description.AddProvidedOptionalFile(TextSearchIndex::TEXT_POI_DAT);
    description.AddProvidedOptionalFile(TextSearchIndex::TEXT_LOC_DAT);
    description.AddProvidedOptionalFile(TextSearchIndex::TEXT_REGION_DAT);
    description.AddProvidedOptionalFile(TextSearchIndex::TEXT_OTHER_DAT);
  }

  bool TextIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                  const ImportParameter &parameter,
                                  Progress &progress)
  {
    progress.SetAction("Calculating required FileOffset size...");

    uint8_t offsetSizeBytes=GetFileOffsetSizeBytes(parameter,
                                                   progress);

    progress.Info("Using "+std::to_string(offsetSizeBytes)+"-byte offsets");

    // add node text data
    if(!this->AddNodeTextToKeysets(parameter,
                                   progress,
                                   *typeConfig,
                                   offsetSizeBytes)) {
      return false;
    }
    if(!this->AddWayTextToKeysets(parameter,
                                  progress,
                                  *typeConfig,
                                  offsetSizeBytes)) {
      return false;
    }
    if(!this->AddAreaTextToKeysets(parameter,
                                   progress,
                                   *typeConfig,
                                   offsetSizeBytes)) {
      return false;
    }

    // Create a file offset size string to indicate
    // how many bytes are used for offsets in the trie

    // We use an ASCII control character to denote
    // the start of the sz offset key:
    // 0x04: EOT
    std::string offsetSizeBytesStr;
    offsetSizeBytesStr.push_back(4);
    offsetSizeBytesStr+=std::to_string(offsetSizeBytes);

    // build and save tries
    std::vector<marisa::Keyset*> keysets;
    keysets.push_back(&keysetPoi);
    keysets.push_back(&keysetLocation);
    keysets.push_back(&keysetRegion);
    keysets.push_back(&keysetOther);

    std::vector<std::string> trieFiles;
    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        TextSearchIndex::TEXT_POI_DAT));

    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        TextSearchIndex::TEXT_LOC_DAT));

    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        TextSearchIndex::TEXT_REGION_DAT));

    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        TextSearchIndex::TEXT_OTHER_DAT));

    for(size_t i=0; i < keysets.size(); i++) {
      // add sz_offset to the keyset
      keysets[i]->push_back(offsetSizeBytesStr.c_str(),
                            offsetSizeBytesStr.length());

      marisa::Trie trie;
      try {
        trie.build(*(keysets[i]),
                   MARISA_DEFAULT_NUM_TRIES |
                   MARISA_BINARY_TAIL |
                   MARISA_LABEL_ORDER |
                   MARISA_DEFAULT_CACHE);
      }
      catch (const marisa::Exception &ex) {
        std::string errorMsg="Error building:" +trieFiles[i];
        errorMsg.append(ex.what());
        progress.Error(errorMsg);
        return false;
      }

      try {
        trie.save(trieFiles[i].c_str());
      }
      catch (const marisa::Exception &ex) {
        std::string errorMsg="Error saving:" +trieFiles[i];
        errorMsg.append(ex.what());
        progress.Error(errorMsg);
        return false;
      }
    }

    return true;
  }

  uint8_t TextIndexGenerator::GetFileOffsetSizeBytes(const ImportParameter &parameter,
                                                     Progress &progress)
  {
    // node count
    std::string nodesDataFile=
    AppendFileToDir(parameter.GetDestinationDirectory(),
                    "nodes.dat");

    const FileOffset nodesFileSize=GetFileSize(nodesDataFile);

    // way count
    std::string waysDataFile=
    AppendFileToDir(parameter.GetDestinationDirectory(),
                    "ways.dat");

    const FileOffset waysFileSize=GetFileSize(waysDataFile);

    // area count
    std::string areasDataFile=
    AppendFileToDir(parameter.GetDestinationDirectory(),
                    "areas.dat");

    const FileOffset areasFileSize=GetFileSize(areasDataFile);

    // Determine the number of bytes needed to store offsets
    uint8_t minNodeOffsetSizeBytes = BytesNeededToEncodeNumber(nodesFileSize);
    uint8_t minWayOffsetSizeBytes  = BytesNeededToEncodeNumber(waysFileSize);
    uint8_t minAreaOffsetSizeBytes = BytesNeededToEncodeNumber(areasFileSize);

    progress.Info("Node filesize is " + std::to_string(nodesFileSize) + " bytes, "+
                  "req. " + std::to_string(minNodeOffsetSizeBytes) +" bytes");

    progress.Info("Way filesize is " + std::to_string(waysFileSize) + " bytes, "+
                  "req. " + std::to_string(minWayOffsetSizeBytes) +" bytes");

    progress.Info("Area filesize is " + std::to_string(areasFileSize) + " bytes, "+
                  "req. " + std::to_string(minAreaOffsetSizeBytes) +" bytes");

    uint8_t offsetSizeBytes=std::max(minNodeOffsetSizeBytes,minWayOffsetSizeBytes);

    offsetSizeBytes = std::max(offsetSizeBytes,minAreaOffsetSizeBytes);

    return offsetSizeBytes;
  }

  bool TextIndexGenerator::AddNodeTextToKeysets(const ImportParameter &parameter,
                                                Progress &progress,
                                                const TypeConfig &typeConfig,
                                                uint8_t offsetSizeBytes)
  {
    progress.SetAction("Getting node text data");

    NameFeatureValueReader    nameReader(typeConfig);
    NameAltFeatureValueReader nameAltReader(typeConfig);

    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   NodeDataFile::NODES_DAT),
                   FileScanner::Sequential,
                   false);

      uint32_t nodeCount=scanner.ReadUInt32();

      // Iterate through each node and add text
      // data to the corresponding keyset
      for(uint32_t n=1; n <= nodeCount; n++) {
        Node node;

        node.Read(typeConfig,
                  scanner);

        if(!node.GetType()->GetIgnore()) {
          NameFeatureValue    *nameValue=nameReader.GetValue(node.GetFeatureValueBuffer());
          NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(node.GetFeatureValueBuffer());

          if (nameValue==nullptr &&
              nameAltValue==nullptr) {
            continue;
          }

          // Save name attributes of this node
          // in the right keyset
          TypeInfoRef    typeInfo=node.GetType();
          marisa::Keyset *keyset;

          if(typeInfo->GetIndexAsPOI()) {
            keyset = &keysetPoi;
          }
          else if(typeInfo->GetIndexAsLocation()) {
            keyset = &keysetLocation;
          }
          else if(typeInfo->GetIndexAsRegion()) {
            keyset = &keysetRegion;
          }
          else {
            keyset = &keysetOther;
          }

          if(nameValue!=nullptr) {
            AddKeyStr(nameValue->GetName(),
                      offsetSizeBytes,
                      node.GetFileOffset(),
                      refNode,
                      keyset,
                      parameter.GetTextIndexVariant());
          }
          if(nameAltValue!=nullptr) {
            AddKeyStr(nameAltValue->GetNameAlt(),
                      offsetSizeBytes,
                      node.GetFileOffset(),
                      refNode,
                      keyset,
                      parameter.GetTextIndexVariant());
          }
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool TextIndexGenerator::AddWayTextToKeysets(const ImportParameter &parameter,
                                               Progress &progress,
                                               const TypeConfig &typeConfig,
                                               uint8_t offsetSizeBytes)
  {
    progress.SetAction("Getting way text data");

    NameFeatureValueReader    nameReader(typeConfig);
    NameAltFeatureValueReader nameAltReader(typeConfig);
    RefFeatureValueReader     refReader(typeConfig);

    // Open ways.dat
    std::string waysDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        WayDataFile::WAYS_DAT);

    FileScanner scanner;

    try {
      scanner.Open(waysDataFile,
                   FileScanner::Sequential,
                   false);

      uint32_t wayCount=scanner.ReadUInt32();

      // Iterate through each way and add text
      // data to the corresponding keyset
      for(uint32_t n=1; n <= wayCount; n++) {
        Way way;

        way.Read(typeConfig,
                 scanner);

        if(way.GetType()->GetIgnore()) {
          continue;
        }

        NameFeatureValue    *nameValue=nameReader.GetValue(way.GetFeatureValueBuffer());
        NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(way.GetFeatureValueBuffer());
        RefFeatureValue     *refValue=refReader.GetValue(way.GetFeatureValueBuffer());

        if (nameValue==nullptr &&
            nameAltValue==nullptr &&
            refValue==nullptr) {
          continue;
        }

        // Save name attributes of this node
        // in the right keyset
        TypeInfoRef    typeInfo=way.GetType();
        marisa::Keyset *keyset;

        if(typeInfo->GetIndexAsPOI()) {
          keyset = &keysetPoi;
        }
        else if(typeInfo->GetIndexAsLocation()) {
          keyset = &keysetLocation;
        }
        else if(typeInfo->GetIndexAsRegion()) {
          keyset = &keysetRegion;
        }
        else {
          keyset = &keysetOther;
        }

        if(nameValue!=nullptr) {
          AddKeyStr(nameValue->GetName(),
                    offsetSizeBytes,
                    way.GetFileOffset(),
                    refWay,
                    keyset,
                    parameter.GetTextIndexVariant());
        }

        if(nameAltValue!=nullptr) {
          AddKeyStr(nameAltValue->GetNameAlt(),
                    offsetSizeBytes,
                    way.GetFileOffset(),
                    refWay,
                    keyset,
                    parameter.GetTextIndexVariant());
        }

        if(refValue!=nullptr) {
          AddKeyStr(refValue->GetRef(),
                    offsetSizeBytes,
                    way.GetFileOffset(),
                    refWay,
                    keyset,
                    parameter.GetTextIndexVariant());
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool TextIndexGenerator::AddAreaTextToKeysets(const ImportParameter &parameter,
                                                Progress &progress,
                                                const TypeConfig &typeConfig,
                                                uint8_t offsetSizeBytes)
  {
    NameFeatureValueReader    nameReader(typeConfig);
    NameAltFeatureValueReader nameAltReader(typeConfig);

    progress.SetAction("Getting area text data");

    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   false);

      uint32_t areaCount=scanner.ReadUInt32();

      // Iterate through each area and add text
      // data to the corresponding keyset
      for(uint32_t n=1; n <= areaCount; n++) {
        Area area;

        area.Read(typeConfig,
                  scanner);

        // Rings might have different types and names
        // so we check  each ring individually
        for(size_t r=0; r < area.rings.size(); r++) {
          if(area.rings[r].GetType()->GetIgnore()) {
            continue;
          }

          NameFeatureValue    *nameValue=nameReader.GetValue(area.rings[r].GetFeatureValueBuffer());
          NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(area.rings[r].GetFeatureValueBuffer());

          if (nameValue==nullptr &&
              nameAltValue==nullptr) {
            continue;
          }

          TypeInfoRef    areaTypeInfo=area.rings[r].GetType();
          marisa::Keyset *keyset;

          if(areaTypeInfo->GetIndexAsPOI()) {
            keyset = &keysetPoi;
          }
          else if(areaTypeInfo->GetIndexAsLocation()) {
            keyset = &keysetLocation;
          }
          else if(areaTypeInfo->GetIndexAsRegion()) {
            keyset = &keysetRegion;
          }
          else {
            keyset = &keysetOther;
          }

          if (nameValue!=nullptr) {
            AddKeyStr(nameValue->GetName(),
                      offsetSizeBytes,
                      area.GetFileOffset(),
                      refArea,
                      keyset,
                      parameter.GetTextIndexVariant());
          }
          if (nameAltValue!=nullptr) {
            AddKeyStr(nameAltValue->GetNameAlt(),
                      offsetSizeBytes,
                      area.GetFileOffset(),
                      refArea,
                      keyset,
                      parameter.GetTextIndexVariant());
          }
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool TextIndexGenerator::BuildKeyStr(const std::string& text,
                                       uint8_t offsetSizeBytes,
                                       FileOffset offset,
                                       const RefType& reftype,
                                       std::string& keyString) const
  {
    if(text.empty()) {
      return false;
    }

    keyString=text;

    // Use ASCII control characters to denote
    // the start of a file offset:
    // ASCII 0x01 'SOH' - corresponds to refNode
    // ASCII 0x02 'STX' - corresponds to refArea
    // ASCII 0x03 'ETX' - corresponds to refWay

    if(reftype == refNode) {
      char c=static_cast<char>(refNode);
      keyString.push_back(c);
    }
    else if(reftype == refWay) {
      char c=static_cast<char>(refWay);
      keyString.push_back(c);
    }
    else if(reftype == refArea) {
      char c=static_cast<char>(refArea);
      keyString.push_back(c);
    }
    else {
      return false;
    }

    // Write the FileOffset into an 8-byte buffer

    // Note that the order is MSB! This is done to
    // maximize the number of common string overlap
    // in the trie.

    // Consider the offsets
    // 0010, 0011, 0024, 0035
    // A trie would have one common branch for
    // '00', with different edges (1,2,3). If
    // LSB was written first, it would have four
    // branches immediately from its root.

    if (offsetSizeBytes > 0) {
      char* buffer = new char[offsetSizeBytes];
      for (uint8_t i = 0; i < offsetSizeBytes; i++) {
        uint8_t r = offsetSizeBytes - 1 - i;
        buffer[r] = ((offset >> (i * 8)) & 0xff);
      }
      for (uint8_t i = 0; i < offsetSizeBytes; i++) {
        keyString.push_back(buffer[i]);
      }
      delete [] buffer;
    }

    return true;
  }

  bool TextIndexGenerator::AddKeyStr(const std::string& text,
                                     uint8_t offsetSizeBytes,
                                     FileOffset offset,
                                     const RefType& reftype,
                                     marisa::Keyset *keyset,
                                     ImportParameter::TextIndexVariant variant) const
  {
    std::string textNorm = UTF8NormForLookup(text);
    if (variant==ImportParameter::TextIndexVariant::transliterate || variant==ImportParameter::TextIndexVariant::both) {
      std::string textTrans = UTF8Transliterate(textNorm);
      if (variant==ImportParameter::TextIndexVariant::both && textTrans==textNorm) {
        // there is no reason to store the same string twice
        variant = ImportParameter::TextIndexVariant::transliterate;
      }
      std::string keyString;
      if (BuildKeyStr(textTrans,
                      offsetSizeBytes,
                      offset,
                      reftype,
                      keyString)) {
        keyset->push_back(keyString.c_str(),
                          keyString.length());
      } else {
        return false;
      }
    }

    if (variant==ImportParameter::TextIndexVariant::original || variant==ImportParameter::TextIndexVariant::both) {
      std::string keyString;
      if (BuildKeyStr(textNorm,
                      offsetSizeBytes,
                      offset,
                      reftype,
                      keyString)) {
        keyset->push_back(keyString.c_str(),
                          keyString.length());
      } else {
        return false;
      }
    }

    return true;
  }


}
