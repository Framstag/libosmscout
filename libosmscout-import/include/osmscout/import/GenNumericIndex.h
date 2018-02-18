#ifndef OSMSCOUT_IMPORT_GENNUMERICINDEX_H
#define OSMSCOUT_IMPORT_GENNUMERICINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <vector>


#include <osmscout/util/Cache.h>
#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/Progress.h>

#include <osmscout/import/Import.h>

namespace osmscout {

  template <class N,class T>
  class NumericIndexGenerator : public ImportModule
  {
  private:
    std::string description;
    std::string datafile;
    std::string indexfile;

  private:
    void ReadData(const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  T& data) const;

  public:
    NumericIndexGenerator(const std::string& description,
                          const std::string& datafile,
                          const std::string& indexfile);

    ~NumericIndexGenerator() override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };

  template <class N,class T>
  NumericIndexGenerator<N,T>::NumericIndexGenerator(const std::string& description,
                                                    const std::string& datafile,
                                                    const std::string& indexfile)
   : description(description),
     datafile(datafile),
     indexfile(indexfile)
  {
    // no code
  }

  template <class N,class T>
  NumericIndexGenerator<N,T>::~NumericIndexGenerator()
  {
    // no code
  }

  template <class N,class T>
  void NumericIndexGenerator<N,T>::ReadData(const TypeConfig& typeConfig,
                                            FileScanner& scanner,
                                            T& data) const
  {
    data.Read(typeConfig,
              scanner);
  }

  template <class N,class T>
  bool NumericIndexGenerator<N,T>::Import(const TypeConfigRef& typeConfig,
                                          const ImportParameter& parameter,
                                          Progress& progress)
  {
    FileScanner             scanner;
    FileWriter              writer;

    uint32_t                dataCount;

    std::vector<N>          startingIds;
    std::vector<FileOffset> pageStarts;

    std::vector<uint32_t>   indexPageCounts;

    FileOffset              levelsOffset;
    FileOffset              lastLevelPageStartOffset;

    FileOffset              indexPageCountsOffset;
    uint32_t                pageSize=(uint32_t)parameter.GetNumericIndexPageSize();

    //
    // Writing index file
    //

    progress.SetAction(std::string("Generating '")+indexfile+"'");

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  indexfile));

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   datafile),
                   FileScanner::Sequential,true);

      scanner.Read(dataCount);

      writer.WriteNumber(pageSize);       // Size of one index page in bytes
      writer.WriteNumber(dataCount);      // Number of entries in data file

      levelsOffset=writer.GetPos();
      writer.Write((uint32_t)0);        // Number of levels

      lastLevelPageStartOffset=writer.GetPos();
      writer.WriteFileOffset((FileOffset)0);        // Write the starting position of the last page

      indexPageCountsOffset=writer.GetPos();
      writer.WriteFileOffset((FileOffset)0);        // Write the starting position of list of sizes of each index level

      writer.FlushCurrentBlockWithZeros(pageSize);

      progress.Info(std::string("Writing level ")+std::to_string(1)+" ("+std::to_string(dataCount)+" entries)");

      N          lastId=0;
      FileOffset lastPos=0;
      uint32_t   currentPageSize=0;

      for (uint32_t d=0; d<dataCount; d++) {
        progress.SetProgress(d,dataCount);

        FileOffset readPos;
        T          data;

        readPos=scanner.GetPos();

        ReadData(*typeConfig,
                 scanner,
                 data);

        if (d>0) {
          if (data.GetId()<=lastId) {
            progress.Error("Current id "+std::to_string(data.GetId())+" <= last id "+std::to_string(lastId));
          }
          assert(data.GetId()>lastId);
          assert(readPos>lastPos);
        }

        if (currentPageSize>0) {
          char         b1[10];
          char         b2[10];
          N            b1val=data.GetId()-lastId;
          FileOffset   b2val=readPos-lastPos;
          unsigned int b1size;
          unsigned int b2size;


          b1size=EncodeNumber(b1val,b1);
          b2size=EncodeNumber(b2val,b2);

          assert(b1size<=10);
          assert(b2size<=10);

          if (currentPageSize+b1size+b2size>pageSize) {
            // Next entry does not fit, fill rest of index page with zeros
            writer.FlushCurrentBlockWithZeros(pageSize);

            currentPageSize=0;
          }
          else {
            writer.Write(b1,b1size);
            writer.Write(b2,b2size);

            currentPageSize+=b1size+b2size;
          }
        }

        if (currentPageSize==0) {
          FileOffset writePos=writer.GetPos();

          startingIds.push_back(data.GetId());
          pageStarts.push_back(writePos);

          writer.WriteNumber(data.GetId());
          writer.WriteNumber(readPos);

          writePos=writer.GetPos();
          currentPageSize=writePos%pageSize;
        }

        lastId=data.GetId();
        lastPos=readPos;
      }

      writer.FlushCurrentBlockWithZeros(pageSize);
      indexPageCounts.push_back((uint32_t)pageStarts.size());

      while (pageStarts.size()>1) {
        std::vector<N>          si(startingIds);
        std::vector<FileOffset> po(pageStarts);

        startingIds.clear();
        pageStarts.clear();

        progress.Info(std::string("Writing level ")+std::to_string(indexPageCounts.size()+1)+" ("+std::to_string(si.size())+" entries)");

        size_t currentPageSize=0;

        for (size_t i=0; i<si.size(); i++) {
          if (currentPageSize>0) {
            char         b1[10];
            char         b2[10];
            N            b1val=si[i]-si[i-1];
            FileOffset   b2val=po[i]-po[i-1];
            unsigned int b1size;
            unsigned int b2size;

            b1size=EncodeNumber(b1val,b1);
            b2size=EncodeNumber(b2val,b2);

            assert(b1size<=10);
            assert(b2size<=10);

            if (currentPageSize+b1size+b2size>pageSize) {
              // Fill rest of first index page with zeros
              writer.FlushCurrentBlockWithZeros(pageSize);

              currentPageSize=0;
            }
            else {
              writer.Write(b1,b1size);
              writer.Write(b2,b2size);

              currentPageSize+=b1size+b2size;
            }
          }

          if (currentPageSize==0) {
            FileOffset writePos;

            writePos=writer.GetPos();

            startingIds.push_back(si[i]);
            pageStarts.push_back(writePos);

            writer.WriteNumber(si[i]);
            writer.WriteNumber(po[i]);

            writePos=writer.GetPos();
            currentPageSize=writePos%pageSize;
          }
        }

        writer.FlushCurrentBlockWithZeros(pageSize);
        indexPageCounts.push_back((uint32_t)pageStarts.size());
      }

      // If we have data to index, we should have at least one root level index page
      if (dataCount>0) {
        assert(pageStarts.size()==1);

        FileOffset indexPageCountsPos;

        indexPageCountsPos=writer.GetPos();

        writer.SetPos(levelsOffset);
        writer.Write((uint32_t)indexPageCounts.size());

        writer.SetPos(lastLevelPageStartOffset);
        writer.WriteFileOffset(pageStarts[0]);

        writer.SetPos(indexPageCountsOffset);
        writer.WriteFileOffset(indexPageCountsPos);

        writer.SetPos(indexPageCountsPos);
      }

      progress.Info(std::string("Index for ")+std::to_string(dataCount)+" data elements will be stored in "+std::to_string(indexPageCounts.size())+ " levels");
      for (size_t level=0; level<indexPageCounts.size(); level++) {
        size_t levelIndex=indexPageCounts.size()-level-1;

        progress.Info(std::string("Page count for level ")+std::to_string(level)+" is "+std::to_string(indexPageCounts[levelIndex]));
        writer.WriteNumber(indexPageCounts[levelIndex]);
      }

      scanner.Close();
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}

#endif
