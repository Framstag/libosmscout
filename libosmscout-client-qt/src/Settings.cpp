/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2013  Tim Teulings

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

#include "osmscout/Settings.h"

Settings::Settings()
{
  // no code
}

Settings::~Settings()
{
  // no code
}

void Settings::SetDPI(size_t dpi)
{
  settings.setValue("settings/dpi", (uint)dpi);
}

size_t Settings::GetDPI() const
{
  return (size_t)settings.value("settings/dpi",92).toUInt();
}

osmscout::Vehicle Settings::GetRoutingVehicle() const
{
  return (osmscout::Vehicle)settings.value("routing/vehicle",osmscout::vehicleCar).toUInt();
}

void Settings::SetRoutingVehicle(const osmscout::Vehicle& vehicle)
{
  settings.setValue("routing/vehicle", (unsigned int)vehicle);
}

