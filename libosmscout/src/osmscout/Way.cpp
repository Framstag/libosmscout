/*
  Import/TravelJinni - Openstreetmap offline viewer
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

#include <osmscout/Way.h>

#include <cassert>
#include <cmath>

namespace osmscout {

  static double conversionFactor=10000000.0;

  bool Way::Read(FileReader& reader)
  {
    unsigned long nodeCount;
    unsigned long flags;


    reader.Read(id);
    reader.ReadNumber(type);
    reader.Read(flags);
    reader.ReadNumber(nodeCount);

    if (reader.HasError()) {
      return false;
    }

    this->flags=flags;
    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      unsigned long latValue;
      unsigned long lonValue;

      reader.Read(nodes[i].id);
      reader.Read(latValue);
      reader.Read(lonValue);

      nodes[i].lat=latValue/conversionFactor-180.0;
      nodes[i].lon=lonValue/conversionFactor-90.0;
    }

    if (flags & hasName) {
      reader.Read(name);
    }

    if (flags & hasRef) {
      reader.Read(ref);
    }

    if (flags & hasLayer) {
      unsigned long layer;

      reader.ReadNumber(layer);

      this->layer=layer;
    }

    if (flags & hasTags) {
      unsigned long tagCount;

      reader.ReadNumber(tagCount);

      if (reader.HasError()) {
        return false;
      }

      tags.resize(tagCount);
      for (size_t i=0; i<tagCount; i++) {
        reader.ReadNumber(tags[i].key);
        reader.Read(tags[i].value);
      }
    }

    if (flags & hasRestrictions) {
      unsigned long restrictionCount;

      reader.ReadNumber(restrictionCount);

      if (reader.HasError()) {
        return false;
      }

      restrictions.resize(restrictionCount);

      for (size_t i=0; i<restrictionCount; i++) {
        unsigned long type;
        unsigned long memberCount;

        reader.ReadNumber(type);
        reader.ReadNumber(memberCount);

        if (reader.HasError()) {
          return false;
        }


        restrictions[i].type=(Way::RestrictionType)type;
        restrictions[i].members.resize(memberCount);

        for (size_t j=0; j<memberCount; j++) {
          reader.Read(restrictions[i].members[j]);
        }
      }
    }

    return !reader.HasError();
  }

  bool Way::Read(FileScanner& scanner)
  {
    unsigned long nodeCount;
    unsigned long flags;

    scanner.Read(id);
    scanner.ReadNumber(type);
    scanner.Read(flags);
    scanner.ReadNumber(nodeCount);

    if (scanner.HasError()) {
      return false;
    }

    this->flags=flags;
    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      unsigned long latValue;
      unsigned long lonValue;

      scanner.Read(nodes[i].id);
      scanner.Read(latValue);
      scanner.Read(lonValue);

      nodes[i].lat=latValue/conversionFactor-180.0;
      nodes[i].lon=lonValue/conversionFactor-90.0;
    }

    if (flags & hasName) {
      scanner.Read(name);
    }

    if (flags & hasRef) {
      scanner.Read(ref);
    }

    if (flags & hasLayer) {
      unsigned long layer;

      scanner.ReadNumber(layer);

      this->layer=layer;
    }

    if (flags & hasTags) {
      unsigned long tagCount;

      scanner.ReadNumber(tagCount);

      if (scanner.HasError()) {
        return false;
      }

      tags.resize(tagCount);
      for (size_t i=0; i<tagCount; i++) {
        scanner.ReadNumber(tags[i].key);
        scanner.Read(tags[i].value);
      }
    }

    if (flags & hasRestrictions) {
      unsigned long restrictionCount;

      scanner.ReadNumber(restrictionCount);

      if (scanner.HasError()) {
        return false;
      }

      restrictions.resize(restrictionCount);

      for (size_t i=0; i<restrictionCount; i++) {
        unsigned long type;
        unsigned long memberCount;

        scanner.ReadNumber(type);
        scanner.ReadNumber(memberCount);

        if (scanner.HasError()) {
          return false;
        }


        restrictions[i].type=(Way::RestrictionType)type;
        restrictions[i].members.resize(memberCount);

        for (size_t j=0; j<memberCount; j++) {
          scanner.Read(restrictions[i].members[j]);
        }
      }
    }

    return !scanner.HasError();
  }

  bool Way::Write(FileWriter& writer) const
  {
    writer.Write(id);
    writer.WriteNumber(type);
    writer.Write((unsigned long)flags);

    writer.WriteNumber(nodes.size());

    for (size_t i=0; i<nodes.size(); i++) {
      unsigned long latValue=(unsigned long)round((nodes[i].lat+180.0)*conversionFactor);
      unsigned long lonValue=(unsigned long)round((nodes[i].lon+90.0)*conversionFactor);

      writer.Write(nodes[i].id);
      writer.Write(latValue);
      writer.Write(lonValue);
    }

    if (flags & hasName) {
      writer.Write(name);
    }

    if (flags & hasRef) {
      writer.Write(ref);
    }

    if (flags & hasLayer) {
      writer.WriteNumber(layer);
    }

    if (flags & hasTags) {
      writer.WriteNumber(tags.size());

      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    if (flags & hasRestrictions) {
      writer.WriteNumber(restrictions.size());

      for (size_t i=0; i<restrictions.size(); i++) {
        writer.WriteNumber(restrictions[i].type);

        writer.WriteNumber(restrictions[i].members.size());

        for (size_t j=0; j<restrictions[i].members.size(); j++) {
          writer.Write(restrictions[i].members[j]);
        }
      }
    }

    return !writer.HasError();
  }
}
