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

#include <osmscout/util/HTMLWriter.h>

#include <osmscout/system/Assert.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  // Remark: Why this ugly exception handling?
  // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66145

  HTMLWriter::HTMLWriter()
  {
    file.exceptions(std::ifstream::failbit|std::ifstream::badbit);
  }

  HTMLWriter::~HTMLWriter()
  {
    if (file.is_open()) {
      log.Warn() << "Automatically closing HTMLWriter for file '" << filename << "'!";
      CloseFailsafe();
    }
  }

  /**
   *
   * @throws IOException
   */
  void HTMLWriter::Open(const std::string& filename)
  {
    this->filename=filename;

    try {
      file.clear();
      file.open(filename);
    }
    catch (std::ifstream::failure& f) {
      throw IOException(filename,"Error opening file for writing",f);
    }
  }

  /**
   *
   * @throws IOException
   */
  void HTMLWriter::Close()
  {
    if (!file.is_open()) {
      throw IOException(filename,"Cannot close file","File already closed");
    }

    try {
      file.close();
    }
    catch (std::ifstream::failure& f) {
      throw IOException(filename,"Cannot close file",f);
    }
  }

  void HTMLWriter::CloseFailsafe()
  {
    if (!file.is_open()) {
      return;
    }

    try {
      file.close();
    }
    catch (std::ifstream::failure&) {
      // suppress any error
    }
  }

  std::string HTMLWriter::Sanitize(const std::string& string) const
  {
    if (string.find_first_of("<>&")!=std::string::npos) {
      std::stringstream buffer;

      for (const char c : string) {
        switch (c) {
        case '<':
          buffer << "&lt;";
          break;
        case '>':
          buffer << "&gt;";
          break;
        case '&':
          buffer << "&amp;";
          break;
        default:
          buffer << c;
          break;
        }
      }

      return buffer.str();
    }
    else {
      return string;
    }
  }

  void HTMLWriter::WriteDocumentStart()
  {
    try {
      file << "<!DOCTYPE html>" << std::endl;
      file << "<html>" << std::endl;
    }
    catch (std::ios_base::failure& e) {
      throw IOException(filename,"Cannot write document start",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write document start",e);
    }
  }

  void HTMLWriter::WriteHeader(const std::string& title,
                               const std::string& description,
                               const std::string& keywords,
                               const std::string& stylesheetLocation)
  {
    try {
      WriteHeaderStart(title,
                       description,
                       keywords,
                       stylesheetLocation);

      WriteHeaderEnd();
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write header",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write header",e);
    }
  }

  void HTMLWriter::WriteHeaderStart(const std::string& title,
                                    const std::string& description,
                                    const std::string& keywords,
                                    const std::string& stylesheetLocation)
  {
    try {
      file << "<head>" << std::endl;

      if (!title.empty()) {
        file << "<title>" << Sanitize(title) << "</title>" << std::endl;
      }

      if (!stylesheetLocation.empty()) {
        file << R"(<link rel="stylesheet" type="text/css" href=")" << stylesheetLocation << "\">" << std::endl;
      }

      file << "<meta charset=\"utf-8\">" << std::endl;

      if (!description.empty()) {
        WriteMeta("description",description);
      }

      if (!keywords.empty()) {
        WriteMeta("keywords",description);
      }

      WriteMeta("generator","libosmscout import");
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write header start",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write header start",e);
    }
  }

  void HTMLWriter::WriteMeta(const std::string& name,
                             const std::string& content)
  {
    try {
      file << "<meta name=\"" << Sanitize(name) << "\" content=\"" << Sanitize(content) << "\"/>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write meta",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write meta",e);
    }
  }

  void HTMLWriter::WriteHeaderEnd()
  {
    try {
      file << "</head>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write header end",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write header end",e);
    }
  }

  void HTMLWriter::WriteBodyStart()
  {
    try {
      file << "<body>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write body start",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write body start",e);
    }
  }

  void HTMLWriter::WriteText(const std::string& text)
  {
    try {
      file << Sanitize(text) << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write text",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write text",e);
    }
  }

  void HTMLWriter::WriteListStart()
  {
    try {
      file << "<ul>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write text",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write text",e);
    }
  }

  void HTMLWriter::WriteListEntryStart()
  {
    try {
      file << "<li>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write text",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write text",e);
    }
  }

  void HTMLWriter::WriteListEntryEnd()
  {
    try {
      file << "</li>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write text",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write text",e);
    }
  }

  void HTMLWriter::WriteListEnd()
  {
    try {
      file << "</ul>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write text",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write text",e);
    }
  }

  void HTMLWriter::WriteLink(const std::string& url,
                             const std::string& title)
  {
    try {
      file << "<a href=\"" << url << "\">" << Sanitize(title) << "</a>";
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write link",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write link",e);
    }
  }

  void HTMLWriter::WriteOSMObjectLink(const ObjectOSMRef& object,
                                      const std::string& name)
  {
    try {
      // TODO: Call WriteLink

      file << "<a href=\"https://www.openstreetmap.org/";

      switch (object.GetType()) {
      case osmRefNone:
        assert(false);
      case osmRefNode:
        file << "node";
        break;
      case osmRefWay:
        file << "way";
        break;
      case osmRefRelation:
        file << "relation";
        break;
      }

      file << "/" << std::to_string(object.GetId()) << "\">" << Sanitize(name) << "</a>";
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write object link",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write object link",e);
    }
  }

  void HTMLWriter::WriteBodyEnd()
  {
    try {
      file << "</body>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write body end",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write body end",e);
    }
  }

  void HTMLWriter::WriteDocumentEnd()
  {
    try {
      file << "</html>" << std::endl;
    }
    catch (std::ifstream::failure& e) {
      throw IOException(filename,"Cannot write document end",e);
    }
    catch (std::exception& e) {
      throw IOException(filename,"Cannot write document end",e);
    }
  }
}

