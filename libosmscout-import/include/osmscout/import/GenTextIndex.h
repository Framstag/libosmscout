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

#include <marisa.h>

#include <osmscout/Types.h>
#include <osmscout/ObjectRef.h>

#include <osmscout/import/Import.h>

#include <osmscout/system/Compiler.h>

namespace osmscout
{
  class TextIndexGenerator CLASS_FINAL : public ImportModule
  {
  public:
    TextIndexGenerator();

    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter &parameter,
                Progress &progress) override;

  private:
    bool SetFileOffsetSize(const ImportParameter &parameter,
                           Progress &progress);

    bool AddNodeTextToKeysets(const ImportParameter &parameter,
                              Progress &progress,
                              const TypeConfig &typeConfig);

    bool AddWayTextToKeysets(const ImportParameter &parameter,
                             Progress &progress,
                             const TypeConfig &typeConfig);

    bool AddAreaTextToKeysets(const ImportParameter &parameter,
                              Progress &progress,
                              const TypeConfig &typeConfig);

    bool BuildKeyStr(const std::string& text,
                     FileOffset offset,
                     const RefType& reftype,
                     std::string& keyString) const;

    // keysets used to store text data and generate tries
    marisa::Keyset  keysetPoi;
    marisa::Keyset  keysetLocation;
    marisa::Keyset  keysetRegion;
    marisa::Keyset  keysetOther;

    uint8_t         offsetSizeBytes;  //! size in bytes of FileOffsets stored in the tries
  };
}


#endif // OSMSCOUT_IMPORT_GENTEXTINDEX_H
