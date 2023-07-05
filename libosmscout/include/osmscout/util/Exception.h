#ifndef OSMSCOUT_UTIL_EXCEPTION_H
#define OSMSCOUT_UTIL_EXCEPTION_H

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

#include <exception>
#include <string>

#include <osmscout/lib/CoreImportExport.h>
#include <system_error>

#if defined(_MSC_VER)
  #if (_MSC_VER <= 1910)
    #include <yvals.h>
    #define OSMSCOUT_NOEXCEPT _NOEXCEPT
  #else
    #define OSMSCOUT_NOEXCEPT noexcept
  #endif
#else
#define OSMSCOUT_NOEXCEPT noexcept
#endif

namespace osmscout {

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable:4275)
#endif

  class OSMSCOUT_API OSMScoutException : public std::exception
  {
  public:
    virtual std::string GetDescription() const;
  };

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

  class OSMSCOUT_API UninitializedException : public OSMScoutException
  {
  private:
    std::string objectName;
    std::string description;

  public:
    explicit UninitializedException(const std::string& objectName);

    const char* what() const OSMSCOUT_NOEXCEPT override;

    std::string GetObjectName() const;
    std::string GetDescription() const override;
  };

  class OSMSCOUT_API IOException : public OSMScoutException
  {
  private:
    std::string filename;
    std::string semanticError;
    std::string errorMsg;
    std::string description;

  public:
    IOException(const std::string& filename,
                const std::string& semanticError,
                const std::system_error& error);
    IOException(const std::string& filename,
                const std::string& semanticError,
                const std::exception& error);
    IOException(const std::string& filename,
                const std::string& semanticError);
    IOException(const std::string& filename,
                const std::string& semanticError,
                const std::string& errorMsg);

    const char* what() const OSMSCOUT_NOEXCEPT override;

    std::string GetFilename() const;
    std::string GetSemanticError() const;
    std::string GetErrorMsg() const;
    std::string GetDescription() const override;
  };
}

#endif
