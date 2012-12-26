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

#include <osmscout/Types.h>

#include <osmscout/private/Math.h>

namespace osmscout {
  const double conversionFactor=10000000.0;

  size_t MagToLevel(double mag)
  {
    return (size_t)log2(mag);
  }


  void Magnification::SetMagnification(double magnification)
  {
    this->magnification=magnification;
    this->level=(size_t)log2(this->magnification);
  }

  void Magnification::SetMagnification(Mag magnification)
  {
    this->magnification=magnification;
    this->level=(size_t)log2(this->magnification);
  }

  void Magnification::SetLevel(size_t level)
  {
    this->magnification=pow(2,level);
    this->level=level;
  }
}
