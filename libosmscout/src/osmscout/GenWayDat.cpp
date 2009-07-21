/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/GenWayDat.h>

#include <fstream>
#include <iostream>

#include <osmscout/Way.h>
#include <osmscout/RawWay.h>

bool GenerateWayDat()
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  std::cout << "Generate ways.dat..." << std::endl;

  std::ifstream in;
  std::ofstream out;

  in.open("rawways.dat",std::ios::in|std::ios::binary);

  if (!in) {
    return false;
  }

  out.open("ways.dat",std::ios::out|std::ios::trunc|std::ios::binary);

  if (!out) {
    return false;
  }

  while (in) {
    RawWay rawWay;
    Way    way;

    rawWay.Read(in);

    if (rawWay.type!=typeIgnore) {
      way.id=rawWay.id;
      way.type=rawWay.type;
      way.flags=rawWay.flags;
      way.layer=rawWay.layer;
      way.tags=rawWay.tags;
      way.nodes.resize(rawWay.nodes.size());
      for (size_t i=0; i<rawWay.nodes.size(); i++) {
        way.nodes[i].id=rawWay.nodes[i];
      }

      way.Write(out);
    }
  }

  in.close();
  out.close();

  return true;
}

