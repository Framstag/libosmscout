#ifndef OSMSCOUT_IMPORT_GENTEXTINDEX_H
#define OSMSCOUT_IMPORT_GENTEXTINDEX_H

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

#include <fstream>

#include <marisa.h>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/ObjectRef.h>

#include <osmscoutimport/Import.h>

#include <osmscout/system/Compiler.h>

namespace osmscout
{
  class TextIndexGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* const FILENAME_TEXT_POI_TXT;
    static const char* const FILENAME_TEXT_LOC_TXT;
    static const char* const FILENAME_TEXT_REGION_TXT;
    static const char* const FILENAME_TEXT_OTHER_TXT;

    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter &parameter,
                Progress &progress) override;

  private:
    uint8_t GetFileOffsetSizeBytes(const ImportParameter &parameter,
                                   Progress &progress);

    bool AddNodeTextToKeysets(const ImportParameter &parameter,
                              Progress &progress,
                              const TypeConfig &typeConfig,
                              uint8_t offsetSizeBytes);

    bool AddWayTextToKeysets(const ImportParameter &parameter,
                             Progress &progress,
                             const TypeConfig &typeConfig,
                             uint8_t offsetSizeBytes);

    bool AddAreaTextToKeysets(const ImportParameter &parameter,
                              Progress &progress,
                              const TypeConfig &typeConfig,
                              uint8_t offsetSizeBytes);

    bool BuildKeyStr(const std::string& text,
                     uint8_t offsetSizeBytes,
                     FileOffset offset,
                     const RefType& reftype,
                     std::string& keyString) const;

    bool AddKeyStr(const std::string& text,
                   uint8_t offsetSizeBytes,
                   FileOffset offset,
                   const RefType& reftype,
                   const std::string& typeName,
                   marisa::Keyset *keyset,
                   std::ostream& debugStream,
                   ImportParameter::TextIndexVariant variant) const;

    // keysets used to store text data and generate tries
    marisa::Keyset  keysetPoi;
    marisa::Keyset  keysetLocation;
    marisa::Keyset  keysetRegion;
    marisa::Keyset  keysetOther;

    // debug streams used to dump the content of the keysets
    std::ofstream   debugStreamPoi;
    std::ofstream   debugStreamLocation;
    std::ofstream   debugStreamRegion;
    std::ofstream   debugStreamOther;
  };
}


#endif // OSMSCOUT_IMPORT_GENTEXTINDEX_H
