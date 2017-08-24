#ifndef OSMSCOUT_UTIL_STRINGMATCHER_H
#define OSMSCOUT_UTIL_STRINGMATCHER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <memory>
#include <string>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  class OSMSCOUT_API StringMatcher
  {
  public:
    enum Result
    {
      noMatch,
      partialMatch,
      match
    };

  public:
    virtual ~StringMatcher() = default;

    virtual Result Match(const std::string& text) const = 0;
  };

  typedef std::shared_ptr<StringMatcher> StringMatcherRef;

  class OSMSCOUT_API StringMatcherCI : public StringMatcher
  {
  private:
    std::string pattern;

  public:
    explicit StringMatcherCI(const std::string& pattern);

    Result Match(const std::string& text) const override;
  };

  class OSMSCOUT_API StringMatcherFactory
  {
  public:
    virtual ~StringMatcherFactory() = default;

    virtual StringMatcherRef CreateMatcher(const std::string& pattern) const = 0;
  };

  typedef std::shared_ptr<StringMatcherFactory> StringMatcherFactoryRef;

  class OSMSCOUT_API StringMatcherCIFactory : public StringMatcherFactory
  {
  public:
    StringMatcherRef CreateMatcher(const std::string& pattern) const override;
  };
}
#endif
