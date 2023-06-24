#ifndef TAGERRORREPORTER_H
#define TAGERRORREPORTER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017 Lukas Karas

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

#include <string>

#include <osmscout/ObjectRef.h>
#include <osmscout/Tag.h>

#include <osmscout/lib/CoreImportExport.h>

namespace osmscout {

  class OSMSCOUT_API TagErrorReporter
  {
  protected:
    TagErrorReporter();

  public:
    virtual ~TagErrorReporter() = default;

    virtual void ReportTag(const ObjectOSMRef& object,
                           const TagMap& tags,
                           const std::string& error) = 0;
  };

  class OSMSCOUT_API SilentTagErrorReporter: public TagErrorReporter
  {
  public:
    SilentTagErrorReporter();

    ~SilentTagErrorReporter() override = default;

    void ReportTag(const ObjectOSMRef& object,
                   const TagMap& tags,
                   const std::string& error) override;
  };
}

#endif /* TAGERRORREPORTER_H */

