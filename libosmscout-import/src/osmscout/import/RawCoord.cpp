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

#include <osmscout/import/RawCoord.h>

namespace osmscout {

  RawCoord::RawCoord()
  : id(0)
  {
    // no code
  }

  void RawCoord::SetOSMId(OSMId id)
  {
    this->id=id;
  }

  void RawCoord::SetCoord(const GeoCoord& coord)
  {
    this->coord=coord;
  }

  /**
   * Reads the data from the given FileScanner
   *
   * @throws IOException
   */
  void RawCoord::Read(const TypeConfig& /*typeConfig*/,
                     FileScanner& scanner)
  {
    scanner.ReadNumber(id);
    scanner.ReadCoord(coord);
  }

  /**
   * Writes the data to the given FileWriter
   *
   * @throws IOException
   */
  void RawCoord::Write(const TypeConfig& /*typeConfig*/,
                      FileWriter& writer) const
  {
    writer.WriteNumber(id);
    writer.WriteCoord(coord);
  }

  /**
   * Writes the data to the given FileWriter
   *
   * @throws IOException
   */
  void RawCoord::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);
    writer.WriteCoord(coord);
  }
}

