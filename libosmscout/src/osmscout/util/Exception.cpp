/*
  This source is part of the libosmscout library
  Copyright (C) 2015  Tim Teulings

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

#include <osmscout/util/Exception.h>

#include <errno.h>
#include <string.h>

namespace osmscout {

  OSMScoutException::~OSMScoutException()
  {
    // no code
  }

  std::string OSMScoutException::GetDescription() const
  {
    return "No error";
  }

  IOException::IOException(const std::string& filename,
                           const std::string& semanticError)
  : filename(filename),
    semanticError(semanticError)
  {
    errorMsg=strerror(errno);
  }

  IOException::IOException(const std::string& filename,
                           const std::string& semanticError,
                           const std::string& errorMsg)
  : filename(filename),
    semanticError(semanticError),
    errorMsg(errorMsg)
  {
    // no code
  }

  std::string IOException::GetFilename() const
  {
    return filename;
  }

  std::string IOException::GetErrorMsg() const
  {
    return errorMsg;
  }

  std::string IOException::GetSemanticError() const
  {
    return semanticError;
  }

  std::string IOException::GetDescription() const
  {
    if (!errorMsg.empty()) {
      return "File '" + filename +"' - " + semanticError+": " + errorMsg;
    }
    else {
      return "File '" + filename +"' - " + semanticError;
    }
  }
}
