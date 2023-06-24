#ifndef OSMSCOUT_TEXTSEARCHINDEX_H
#define OSMSCOUT_TEXTSEARCHINDEX_H

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

#include <unordered_map>

#include <osmscout/ObjectRef.h>

#include <osmscout/io/FileScanner.h>

#include <marisa.h>

namespace osmscout
{
  /**
   \ingroup Database
   A class that allows prefix-based searching
   of text data indexed during import
   */
  class OSMSCOUT_API TextSearchIndex final
  {
  public:
    static const char* const TEXT_POI_DAT;
    static const char* const TEXT_LOC_DAT;
    static const char* const TEXT_REGION_DAT;
    static const char* const TEXT_OTHER_DAT;

  private:
    struct TrieInfo
    {
      marisa::Trie *trie=nullptr;
      std::string  file;
      bool         isAvail=false;
    };

  public:
    using ResultsMap = std::unordered_map<std::string, std::vector<ObjectFileRef> >;

    TextSearchIndex() = default;

    ~TextSearchIndex();

    bool Load(const std::string &path);

    bool Search(const std::string& query,
                bool searchPOIs,
                bool searchLocations,
                bool searchRegions,
                bool searchOther,
                bool transliterate,
                ResultsMap& results) const;

  private:
    void splitSearchResult(const std::string& result,
                           std::string& text,
                           ObjectFileRef& ref) const;


    uint8_t               offsetSizeBytes;  //! size in bytes of FileOffsets stored in the tries
    std::vector<TrieInfo> tries;
  };
}


#endif // OSMSCOUT_TEXTSEARCHINDEX_H
