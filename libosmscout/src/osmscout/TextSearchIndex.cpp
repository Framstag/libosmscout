#include <osmscout/TextSearchIndex.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>

namespace osmscout
{
  const char* TextSearchIndex::TEXT_POI_DAT="textpoi.dat";
  const char* TextSearchIndex::TEXT_LOC_DAT="textloc.dat";
  const char* TextSearchIndex::TEXT_REGION_DAT="textregion.dat";
  const char* TextSearchIndex::TEXT_OTHER_DAT="textother.dat";

  TextSearchIndex::TextSearchIndex()
  {
    // no code
  }

  TextSearchIndex::~TextSearchIndex()
  {
    for (size_t i=0; i<tries.size(); i++) {
      tries[i].isAvail=false;
      if (tries[i].trie) {
        delete tries[i].trie;
        tries[i].trie=nullptr;
      }
    }
  }

  bool TextSearchIndex::Load(const std::string& path)
  {
    std::string fixedPath=path;
    if (path[path.length()-1]!='/') {
      fixedPath.push_back('/');
    }

    TrieInfo trie;
    trie.file=AppendFileToDir(fixedPath,TEXT_POI_DAT);
    tries.push_back(trie);

    trie.file=AppendFileToDir(fixedPath,TEXT_LOC_DAT);
    tries.push_back(trie);

    trie.file=AppendFileToDir(fixedPath,TEXT_REGION_DAT);
    tries.push_back(trie);

    trie.file=AppendFileToDir(fixedPath,TEXT_OTHER_DAT);
    tries.push_back(trie);

    uint8_t triesAvail=0;
    for (size_t i=0; i<tries.size(); i++) {
      // open/load the data file
      try {
        triesAvail++;
        tries[i].isAvail=true;
        tries[i].trie=new marisa::Trie;
        tries[i].trie->load(tries[i].file.c_str());
      }
      catch (const marisa::Exception &ex) {
        // We don't return false on a failed load attempt
        // since its possible that the user does not want
        // to include a specific trie (ie. textother)
        log.Error() << "Warn, could not open " << tries[i].file << ":"  << ex.what();
        delete tries[i].trie;
        tries[i].trie=nullptr;
        tries[i].isAvail=false;
        triesAvail--;
      }
    }

    if (triesAvail==0) {
      log.Error() << "TextSearchIndex: No valid text data files is available";

      return false;
    }

    // Determine the number of bytes used for offsets
    for (auto& trie : tries) {
      if (trie.isAvail) {
        // We use an ASCII control character to denote
        // the start of the sz offset key:
        // 0x04: EOT
        std::string offsetSizeBytesQuery;
        offsetSizeBytesQuery.push_back(4);

        marisa::Agent agent;
        agent.set_query(offsetSizeBytesQuery.c_str(),
                        offsetSizeBytesQuery.length());

        // there should only be one result
        if (trie.trie->predictive_search(agent)) {
          std::string result(agent.key().ptr(),agent.key().length());
          result.erase(0,1);  // get rid of the ASCII control char
          if (!StringToNumberUnsigned(result,offsetSizeBytes)) {
            log.Error() << "Could not parse file offset size in text data";

            return false;
          }
          break;
        }
        else {
          log.Error() << "Could not find file offset size in text data";

          return false;
        }
      }
    }

    return true;
  }

  bool TextSearchIndex::Search(const std::string& query,
                               bool searchPOIs,
                               bool searchLocations,
                               bool searchRegions,
                               bool searchOther,
                               ResultsMap& results) const
  {
    results.clear();

    if (query.empty()) {
      return true;
    }

    std::vector<bool> searchGroups;

    searchGroups.push_back(searchPOIs);
    searchGroups.push_back(searchLocations);
    searchGroups.push_back(searchRegions);
    searchGroups.push_back(searchOther);

    for (size_t i=0; i<tries.size(); i++) {
      if (searchGroups[i] && tries[i].isAvail) {
        marisa::Agent agent;

        try {
          agent.set_query(query.c_str(),
                          query.length());
          while (tries[i].trie->predictive_search(agent)) {
            std::string   result(agent.key().ptr(),
                                 agent.key().length());
            std::string   text;
            ObjectFileRef ref;

            splitSearchResult(result,text,ref);

            ResultsMap::iterator it=results.find(text);
            if (it==results.end()) {
              // If the text has not been added to the
              // search results yet, insert a new entry
              std::pair<std::string,std::vector<ObjectFileRef>> entry;
              entry.first=text;
              entry.second.push_back(ref);
              results.insert(entry);
            }
            else {
              // Else add the offset to the existing entry
              it->second.push_back(ref);
            }
          }
        }
        catch (const marisa::Exception &ex) {
          log.Error() << "Error searching for text: " << ex.what();

          return false;
        }
      }
    }

    return true;
  }

  void TextSearchIndex::splitSearchResult(const std::string& result,
                                          std::string& text,
                                          ObjectFileRef& ref) const
  {
    // Get the index that marks the end of the
    // the text and where the FileOffset begins

    // Each result has only one offset that occupies
    // the last offsetSizeBytes bytes of the string, and
    // the offset is in MSB left to right

    FileOffset offset=0;
    FileOffset add=0;
    size_t     idx=result.size()-1;

    for (size_t i=0; i<offsetSizeBytes; i++) {
      add=(unsigned char)(result[idx]);
      offset|=(add << (i*8));

      idx--;
    }

    // Immediately preceding the FileOffset is
    // a single byte that denotes offset type
    RefType reftype=static_cast<RefType>((unsigned char)(result[idx]));

    ref.Set(offset,reftype);
    text=result.substr(0,idx);
  }
}
