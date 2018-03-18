/*
  This source is part of the libosmscout library
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

#include <limits>

#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  bool Way::GetCenter(GeoCoord& center) const
  {
    if (nodes.empty()) {
      return false;
    }

    double minLat=nodes[0].GetLat();
    double minLon=nodes[0].GetLon();
    double maxLat=nodes[0].GetLat();
    double maxLon=nodes[0].GetLon();

    for (size_t i=1; i<nodes.size(); i++) {
      minLat=std::min(minLat,nodes[i].GetLat());
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLat=std::max(maxLat,nodes[i].GetLat());
      maxLon=std::max(maxLon,nodes[i].GetLon());
    }

    center.Set(minLat+(maxLat-minLat)/2,
               minLon+(maxLon-minLon)/2);

    return true;
  }

  void Way::SetLayerToMax()
  {
    // TODO
    // attributes.SetLayer(std::numeric_limits<int8_t>::max());
  }

  bool Way::GetNodeIndexByNodeId(Id id,
                                 size_t& index) const
  {
    for (size_t i=0; i<nodes.size(); i++) {
      if (nodes[i].GetId()==id) {
        index=i;

        return true;
      }
    }

    return false;
  }

  /**
   * Read the data from the given FileScanner.
   *
   * @throws IOException
   */
  void Way::Read(const TypeConfig& typeConfig,
                 FileScanner& scanner)
  {
    TypeId typeId;

    fileOffset=scanner.GetPos();

    scanner.ReadTypeId(typeId,
                       typeConfig.GetWayTypeIdBytes());

    TypeInfoRef type=typeConfig.GetWayTypeInfo(typeId);

    featureValueBuffer.SetType(type);

    featureValueBuffer.Read(scanner);

    scanner.Read(nodes,
                 type->CanRoute() ||
                 type->GetOptimizeLowZoom());
    nextFileOffset=scanner.GetPos();
  }

  /**
   * Read the data from the given FileScanner. Node Ids are not read.
   *
   * @throws IOException
   */
  void Way::ReadOptimized(const TypeConfig& typeConfig,
                          FileScanner& scanner)
  {
    TypeId typeId;

    fileOffset=scanner.GetPos();

    scanner.ReadTypeId(typeId,
                       typeConfig.GetWayTypeIdBytes());

    featureValueBuffer.SetType(typeConfig.GetWayTypeInfo(typeId));

    featureValueBuffer.Read(scanner);

    scanner.Read(nodes,false);
    nextFileOffset=scanner.GetPos();
  }

  /**
   * Writes the data to the given FileWriter.
   *
   * @throws IOException
   */
  void Way::Write(const TypeConfig& typeConfig,
                  FileWriter& writer) const
  {
    assert(!nodes.empty());

    writer.WriteTypeId(featureValueBuffer.GetType()->GetWayId(),
                       typeConfig.GetWayTypeIdBytes());

    featureValueBuffer.Write(writer);

    writer.Write(nodes,
                 featureValueBuffer.GetType()->CanRoute() ||
                 featureValueBuffer.GetType()->GetOptimizeLowZoom());
  }

  /**
   * Writes the data to the given FileWriter. Node Ids are not written.
   *
   * @throws IOException
   */
  void Way::WriteOptimized(const TypeConfig& typeConfig,
                           FileWriter& writer) const
  {
    assert(!nodes.empty());

    writer.WriteTypeId(featureValueBuffer.GetType()->GetWayId(),
                       typeConfig.GetWayTypeIdBytes());

    featureValueBuffer.Write(writer);

    writer.Write(nodes,false);
  }
}
