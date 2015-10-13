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

#include <osmscout/Way.h>
#include <osmscout/Node.h>
#include <osmscout/Area.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

#include <osmscout/import/GenTextIndex.h>

#include <marisa.h>

namespace osmscout
{
  TextIndexGenerator::TextIndexGenerator() :
    offsetSizeBytes(4)
  {
    // no code
  }

  std::string TextIndexGenerator::GetDescription() const
  {
    return "Generate text data files 'text(poi,loc,region,other).dat'";
  }


  bool TextIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                  const ImportParameter &parameter,
                                  Progress &progress)
  {
    if(!this->SetFileOffsetSize(parameter,
                                progress)) {
      return false;
    }
    progress.Info("Using "+NumberToString(offsetSizeBytes)+"-byte offsets");

    // add node text data
    if(!this->AddNodeTextToKeysets(parameter,
                                   progress,
                                   *typeConfig)) {
      return false;
    }
    if(!this->AddWayTextToKeysets(parameter,
                                  progress,
                                  *typeConfig)) {
      return false;
    }
    if(!this->AddAreaTextToKeysets(parameter,
                                   progress,
                                   *typeConfig)) {
      return false;
    }

    // Create a file offset size string to indicate
    // how many bytes are used for offsets in the trie

    // We use an ASCII control character to denote
    // the start of the sz offset key:
    // 0x04: EOT
    std::string offsetSizeBytesStr;
    offsetSizeBytesStr.push_back(4);
    offsetSizeBytesStr+=NumberToString(offsetSizeBytes);

    // build and save tries
    std::vector<marisa::Keyset*> keysets;
    keysets.push_back(&keysetPoi);
    keysets.push_back(&keysetLocation);
    keysets.push_back(&keysetRegion);
    keysets.push_back(&keysetOther);

    std::vector<std::string> trieFiles;
    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "textpoi.dat"));

    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "textloc.dat"));

    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "textregion.dat"));

    trieFiles.push_back(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "textother.dat"));

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
      catch (const marisa::Exception &ex){
        std::string errorMsg="Error building:" +trieFiles[i];
        errorMsg.append(ex.what());
        progress.Error(errorMsg);
        return false;
      }

      try {
        trie.save(trieFiles[i].c_str());
      }
      catch (const marisa::Exception &ex){
        std::string errorMsg="Error saving:" +trieFiles[i];
        errorMsg.append(ex.what());
        progress.Error(errorMsg);
        return false;
      }
    }

    return true;
  }

  bool TextIndexGenerator::SetFileOffsetSize(const ImportParameter &parameter,
                                             Progress &progress)
  {
    progress.SetAction("Calculating required FileOffset size...");

    FileScanner scanner;

    FileOffset nodesFileSize=0;
    FileOffset waysFileSize=0;
    FileOffset areasFileSize=0;

    // node count
    std::string nodesDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        "nodes.dat");

    if (!GetFileSize(nodesDataFile,
                     nodesFileSize)) {
      progress.Error("Cannot get file size of file 'nodes.dat'");
      return false;
    }

    // way count
    std::string waysDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        "ways.dat");

    if (!GetFileSize(waysDataFile,
                     waysFileSize)) {
      progress.Error("Cannot get file size of file 'ways.dat'");
      return false;
    }

    // area count
    std::string areasDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        "areas.dat");

    if (!GetFileSize(areasDataFile,
                     areasFileSize)) {
      progress.Error("Cannot get file size of file 'areas.dat'");
      return false;
    }

    // Determine the number of bytes needed to store offsets
    uint8_t minNodeOffsetSizeBytes = BytesNeededToEncodeNumber(nodesFileSize);
    uint8_t minWayOffsetSizeBytes  = BytesNeededToEncodeNumber(waysFileSize);
    uint8_t minAreaOffsetSizeBytes = BytesNeededToEncodeNumber(areasFileSize);

    progress.Info("Node filesize is " + NumberToString(nodesFileSize) + " bytes, "+
                  "req. " + NumberToString(minNodeOffsetSizeBytes) +" bytes");

    progress.Info("Way filesize is " + NumberToString(waysFileSize) + " bytes, "+
                  "req. " + NumberToString(minWayOffsetSizeBytes) +" bytes");

    progress.Info("Area filesize is " + NumberToString(areasFileSize) + " bytes, "+
                  "req. " + NumberToString(minAreaOffsetSizeBytes) +" bytes");

    offsetSizeBytes = 0;
    offsetSizeBytes = std::max(minNodeOffsetSizeBytes,minWayOffsetSizeBytes);
    offsetSizeBytes = std::max(offsetSizeBytes,minAreaOffsetSizeBytes);

    return true;
  }

  bool TextIndexGenerator::AddNodeTextToKeysets(const ImportParameter &parameter,
                                                Progress &progress,
                                                const TypeConfig &typeConfig)
  {
    progress.SetAction("Getting node text data");

    NameFeatureValueReader    nameReader(typeConfig);
    NameAltFeatureValueReader nameAltReader(typeConfig);

    // Open nodes.dat
    std::string nodesDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        "nodes.dat");

    FileScanner scanner;
    if(!scanner.Open(nodesDataFile,
                     FileScanner::Sequential,
                     false)) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    uint32_t nodeCount=0;
    if(!scanner.Read(nodeCount)) {
      progress.Error("Error reading node count in 'nodes.dat'");
      return false;
    }

    // Iterate through each node and add text
    // data to the corresponding keyset
    for(uint32_t n=1; n <= nodeCount; n++) {
      Node node;
      if (!node.Read(typeConfig,
                     scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(nodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if(!node.GetType()->GetIgnore()) {
        NameFeatureValue    *nameValue=nameReader.GetValue(node.GetFeatureValueBuffer());
        NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(node.GetFeatureValueBuffer());

        if (nameValue==NULL &&
            nameAltValue==NULL) {
          continue;
        }

        // Save name attributes of this node
        // in the right keyset
        TypeInfoRef typeInfo=node.GetType();
        marisa::Keyset * keyset;
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

        if(nameValue!=NULL) {
          std::string keyString;
          if(BuildKeyStr(nameValue->GetName(),
                         node.GetFileOffset(),
                         refNode,
                         keyString))
          {
            keyset->push_back(keyString.c_str(),
                              keyString.length());
          }
        }
        if(nameAltValue!=NULL) {
          std::string keyString;
          if(BuildKeyStr(nameAltValue->GetNameAlt(),
                         node.GetFileOffset(),
                         refNode,
                         keyString))
          {
            keyset->push_back(keyString.c_str(),
                              keyString.length());
          }
        }
      }
    }

    return scanner.Close();
  }

  bool TextIndexGenerator::AddWayTextToKeysets(const ImportParameter &parameter,
                                               Progress &progress,
                                               const TypeConfig &typeConfig)
  {
    progress.SetAction("Getting way text data");

    NameFeatureValueReader    nameReader(typeConfig);
    NameAltFeatureValueReader nameAltReader(typeConfig);
    RefFeatureValueReader     refReader(typeConfig);

    // Open ways.dat
    std::string waysDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        "ways.dat");

    FileScanner scanner;
    if(!scanner.Open(waysDataFile,
                     FileScanner::Sequential,
                     false)) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    uint32_t wayCount=0;
    if(!scanner.Read(wayCount)) {
      progress.Error("Error reading way count in 'ways.dat'");
      return false;
    }

    // Iterate through each way and add text
    // data to the corresponding keyset
    for(uint32_t n=1; n <= wayCount; n++) {
      Way way;
      if (!way.Read(typeConfig,
                    scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if(way.GetType()->GetIgnore()) {
        continue;
      }

      NameFeatureValue    *nameValue=nameReader.GetValue(way.GetFeatureValueBuffer());
      NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(way.GetFeatureValueBuffer());
      RefFeatureValue     *refValue=refReader.GetValue(way.GetFeatureValueBuffer());

      if (nameValue==NULL &&
          nameAltValue==NULL &&
          refValue==NULL) {
        continue;
      }

      // Save name attributes of this node
      // in the right keyset
      TypeInfoRef typeInfo=way.GetType();
      marisa::Keyset * keyset;

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

      if(nameValue!=NULL) {
        std::string keyString;
        if(BuildKeyStr(nameValue->GetName(),
                       way.GetFileOffset(),
                       refWay,
                       keyString))
        {
          keyset->push_back(keyString.c_str(),
                            keyString.length());
        }
      }

      if(nameAltValue!=NULL) {
        std::string keyString;
        if(BuildKeyStr(nameAltValue->GetNameAlt(),
                       way.GetFileOffset(),
                       refWay,
                       keyString))
        {
          keyset->push_back(keyString.c_str(),
                            keyString.length());
        }
      }

      if(refValue!=NULL) {
        std::string keyString;
        if(BuildKeyStr(refValue->GetRef(),
                       way.GetFileOffset(),
                       refWay,
                       keyString))
        {
          keyset->push_back(keyString.c_str(),
                            keyString.length());
        }
      }
    }

    return scanner.Close();
  }

  bool TextIndexGenerator::AddAreaTextToKeysets(const ImportParameter &parameter,
                                                Progress &progress,
                                                const TypeConfig &typeConfig)
  {
    NameFeatureValueReader    nameReader(typeConfig);
    NameAltFeatureValueReader nameAltReader(typeConfig);

    progress.SetAction("Getting area text data");

    // Open areas.dat
    std::string areasDataFile=
        AppendFileToDir(parameter.GetDestinationDirectory(),
                        "areas.dat");

    FileScanner scanner;
    if(!scanner.Open(areasDataFile,
                     FileScanner::Sequential,
                     false)) {
      progress.Error("Cannot open 'areas.dat'");
      return false;
    }

    uint32_t areaCount=0;
    if(!scanner.Read(areaCount)) {
      progress.Error("Error reading area count in 'areas.dat'");
      return false;
    }

    // Iterate through each area and add text
    // data to the corresponding keyset
    for(uint32_t n=1; n <= areaCount; n++) {
      Area area;
      if(!area.Read(typeConfig,
                    scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      // Rings might have different types and names
      // so we check  each ring individually
      for(size_t r=0; r < area.rings.size(); r++) {
        if(area.rings[r].GetType()->GetIgnore()) {
          continue;
        }

        NameFeatureValue    *nameValue=nameReader.GetValue(area.rings[r].GetFeatureValueBuffer());
        NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(area.rings[r].GetFeatureValueBuffer());

        if (nameValue==NULL &&
            nameAltValue==NULL) {
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

        if (nameValue!=NULL) {
          std::string keyString;
          if(BuildKeyStr(nameValue->GetName(),
                         area.GetFileOffset(),
                         refArea,
                         keyString))
          {
            keyset->push_back(keyString.c_str(),
                              keyString.length());
          }
        }
        if (nameAltValue!=NULL) {
          std::string keyString;
          if(BuildKeyStr(nameAltValue->GetNameAlt(),
                         area.GetFileOffset(),
                         refArea,
                         keyString))
          {
            keyset->push_back(keyString.c_str(),
                              keyString.length());
          }
        }
      }
    }

    return scanner.Close();
  }

  bool TextIndexGenerator::BuildKeyStr(const std::string &text,
                                       const FileOffset offset,
                                       const RefType reftype,
                                       std::string &keyString) const
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

    char buffer[offsetSizeBytes];
    for(uint8_t i=0; i < offsetSizeBytes; i++) {
      uint8_t r=offsetSizeBytes-1-i;
      buffer[r] = ((offset >> (i*8)) & 0xff);
    }

    for(uint8_t i=0; i < offsetSizeBytes; i++) {
      keyString.push_back(buffer[i]);
    }

    return true;
  }
}
