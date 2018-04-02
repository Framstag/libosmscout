#ifndef OSMSCOUT_HTMLWRITER_H
#define OSMSCOUT_HTMLWRITER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscout/CoreImportExport.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/util/Exception.h>

namespace osmscout {

  /**
    \ingroup File

    HTMLWriter allows easy generation of HTML web pages containing OSM based reports.
    It not only allows writing simple HTML primitives but also offers higher level
    methods.
    */
  class OSMSCOUT_API HTMLWriter
  {
  private:
    std::ofstream file;     //!< The internal file handle
    std::string   filename; //!< The filename

  public:
    HTMLWriter();
    virtual ~HTMLWriter();

    void Open(const std::string& filename);
    void Close();
    void CloseFailsafe();

    inline bool IsOpen() const
    {
      return file.is_open();
    }

    inline bool HasError() const
    {
      return file.fail();
    }

    inline std::string GetFilename() const
    {
      return filename;
    }

    std::string Sanitize(const std::string& string) const;

    void WriteDocumentStart();
    void WriteHeader(const std::string& title,
                     const std::string& description,
                     const std::string& keywords,
                     const std::string& stylesheetLocation);
    void WriteHeaderStart(const std::string& title,
                          const std::string& description,
                          const std::string& keywords,
                          const std::string& stylesheetLocation);
    void WriteMeta(const std::string& name,
                   const std::string& content);
    void WriteHeaderEnd();

    void WriteBodyStart();

    void WriteText(const std::string& text);

    void WriteListStart();
    void WriteListEntryStart();
    void WriteListEntryEnd();
    void WriteListEnd();

    void WriteLink(const std::string& url,
                   const std::string& title);

    void WriteOSMObjectLink(const ObjectOSMRef& object,
                            const std::string& name);

    void WriteBodyEnd();

    void WriteDocumentEnd();
  };
}

#endif
