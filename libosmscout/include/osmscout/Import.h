#ifndef OSMSCOUT_IMPORT_H
#define OSMSCOUT_IMPORT_H

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

#include <string>

#include <osmscout/Progress.h>

class ImportParameter
{
private:
  std::string mapfile;
  size_t      startStep;
  size_t      endStep;
  size_t      nodeIndexIntervalSize;
  size_t      wayIndexIntervalSize;

public:
  ImportParameter();

  std::string GetMapfile() const;
  size_t GetStartStep() const;
  size_t GetEndStep() const;
  size_t GetNodeIndexIntervalSize() const;
  size_t GetWayIndexIntervalSize() const;

  void SetMapfile(const std::string& mapfile);
  void SetStartStep(size_t startStep);
  void SetSteps(size_t startStep, size_t endStep);
  void SetNodeIndexIntervalSize(size_t nodeIndexIntervalSize);
  void SetWayIndexIntervalSize(size_t wayIndexIntervalSize);
};

extern bool Import(const ImportParameter& parameter,
                   Progress& progress);

#endif
