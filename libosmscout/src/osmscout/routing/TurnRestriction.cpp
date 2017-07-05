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

#include <osmscout/routing/TurnRestriction.h>

namespace osmscout {

  /**
   * Reads the TurnRestriction data from the given FileScanner
   *
   * @throws IOException
   */
  void TurnRestriction::Read(FileScanner& scanner)
  {
    uint32_t typeValue;

    scanner.ReadNumber(typeValue);
    this->type=(Type)typeValue;

    scanner.ReadNumber(from);
    scanner.ReadNumber(via);
    scanner.ReadNumber(to);
  }

  /**
   * Write the TurnRestriction data to the given FileWriter
   *
   * @throws IOException
   */
  void TurnRestriction::Write(FileWriter& writer) const
  {
    writer.WriteNumber((uint32_t)type);

    writer.WriteNumber(from);
    writer.WriteNumber(via);
    writer.WriteNumber(to);
  }
}
