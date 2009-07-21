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

#include <cmath>
#include <iostream>
static double conversionFactor=10000000.0;

static TagId    scratchTags[1024];
static Id       scratchIds[1024];
static uint32_t scratchCoords[2*1024];

void Way::Read(std::istream& file)
{
  uint8_t  tagCount;
  uint16_t nodeCount;

  file.read((char*)&id,sizeof(id));

  if (!file) {
    return;
  }

  file.read((char*)&type,sizeof(type));
  file.read((char*)&flags,sizeof(flags));

  if (flags & hasLayer) {
    file.read((char*)&layer,sizeof(layer));
  }

  file.read((char*)&tagCount,sizeof(tagCount));
  tags.resize(tagCount);

  // To safe read calls we try to bulk load upto 1024 tag ids
  if (tagCount<=1024) {
    file.read((char*)&scratchTags,sizeof(TagId)*tagCount);

    for (size_t i=0; i<tagCount; i++) {
      tags[i].key=scratchTags[i];
    }
  }
  else {
    for (size_t i=0; i<tagCount; i++) {
      file.read((char*)&tags[i].key,sizeof(tags[i].key));
    }

  }

  for (size_t i=0; i<tagCount; i++) {
    std::getline(file,tags[i].value,'\0');
  }

  file.read((char*)&nodeCount,sizeof(nodeCount));
  nodes.resize(nodeCount);

  // To safe read calls we try to bulk load upto 1024 points
  if (nodeCount<=1024) {
    file.read((char*)&scratchIds,sizeof(Id)*nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      nodes[i].id=scratchIds[i];
    }


    file.read((char*)&scratchCoords,2*sizeof(uint32_t)*nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      nodes[i].lat=scratchCoords[2*i]/conversionFactor-180.0;
      nodes[i].lon=scratchCoords[2*i+1]/conversionFactor-90.0;
    }
  }
  else {
    for (size_t i=0; i<nodeCount; i++) {
      file.read((char*)&nodes[i].id,sizeof(nodes[i].id));
    }

    for (size_t i=0; i<nodeCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;

      file.read((char*)&latValue,sizeof(latValue));
      file.read((char*)&lonValue,sizeof(lonValue));

      nodes[i].lat=latValue/conversionFactor-180.0;
      nodes[i].lon=lonValue/conversionFactor-90.0;
    }
  }
}

void Way::Write(std::ostream& file) const
{
  uint8_t  tagCount=tags.size();
  uint16_t nodeCount=nodes.size();

  file.write((const char*)&id,sizeof(id));
  file.write((const char*)&type,sizeof(type));
  file.write((const char*)&flags,sizeof(flags));

  if (flags & hasLayer) {
    file.write((const char*)&layer,sizeof(layer));
  }

  file.write((const char*)&tagCount,sizeof(tagCount));

  for (size_t i=0; i<tags.size(); i++) {
    file.write((const char*)&tags[i].key,sizeof(tags[i].key));
  }


  for (size_t i=0; i<tags.size(); i++) {
    file << tags[i].value << '\0';
  }

  file.write((const char*)&nodeCount,sizeof(nodeCount));

  for (size_t i=0; i<nodes.size(); i++) {
    file.write((const char*)&nodes[i].id,sizeof(nodes[i].id));
  }

  for (size_t i=0; i<nodes.size(); i++) {
    uint32_t latValue=round((nodes[i].lat+180.0)*conversionFactor);
    uint32_t lonValue=round((nodes[i].lon+90.0)*conversionFactor);

    file.write((const char*)&latValue,sizeof(latValue));
    file.write((const char*)&lonValue,sizeof(lonValue));
  }
}

std::string Way::GetName() const
{
  for (size_t i=0; i<tags.size(); i++) {
    if (tags[i].key==tagName) {
      return tags[i].value;
    }
  }

  return "";
}

std::string Way::GetRefName() const
{
  for (size_t i=0; i<tags.size(); i++) {
    if (tags[i].key==tagRef) {
      return tags[i].value;
    }
  }

  return "";
}

