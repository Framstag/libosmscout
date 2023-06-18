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

#if !defined(_MSC_VER)
#include <errno.h>
#endif
#include <cstring>

namespace osmscout {

  std::string OSMScoutException::GetDescription() const
  {
    return "No error";
  }

  UninitializedException::UninitializedException(const std::string& objectName)
  : objectName(objectName)
  {
    description="Object '"+objectName+"' is not initialized";
  }

  const char* UninitializedException::what() const OSMSCOUT_NOEXCEPT
  {
    return description.c_str();
  }

  std::string UninitializedException::GetObjectName() const
  {
    return objectName;
  }

  std::string UninitializedException::GetDescription() const
  {
    return description;
  }

  IOException::IOException(const std::string& filename,
                           const std::string& semanticError,
                           const std::system_error& error)
    : filename(filename),
      semanticError(semanticError),
      errorMsg(error.code().message())
  {

    if (!errorMsg.empty()) {
      description="File '" + filename +"' - " + semanticError+": " + errorMsg;
    }
    else {
      description="File '" + filename +"' - " + semanticError;
    }
  }

  IOException::IOException(const std::string& filename,
                           const std::string& semanticError,
                           const std::exception& error)
    : filename(filename),
      semanticError(semanticError),
      errorMsg(error.what())
  {
    if (!errorMsg.empty()) {
      description="File '" + filename +"' - " + semanticError+": " + errorMsg;
    }
    else {
      description="File '" + filename +"' - " + semanticError;
    }
  }

  IOException::IOException(const std::string& filename,
                           const std::string& semanticError)
  : filename(filename),
    semanticError(semanticError)
  {
    errorMsg=strerror(errno);

    if (!errorMsg.empty()) {
      description="File '" + filename +"' - " + semanticError+": " + errorMsg;
    }
    else {
      description="File '" + filename +"' - " + semanticError;
    }
  }

  IOException::IOException(const std::string& filename,
                           const std::string& semanticError,
                           const std::string& errorMsg)
  : filename(filename),
    semanticError(semanticError),
    errorMsg(errorMsg)
  {
    if (!errorMsg.empty()) {
      description="File '" + filename +"' - " + semanticError+": " + errorMsg;
    }
    else {
      description="File '" + filename +"' - " + semanticError;
    }
  }

  const char* IOException::what() const OSMSCOUT_NOEXCEPT
  {
    return description.c_str();
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
    return description;
  }
}
