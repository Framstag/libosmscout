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

#include <osmscout/Import.h>

#include <iostream>

#include <osmscout/Progress.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeConfigLoader.h>

#include <osmscout/Node.h>
#include <osmscout/Way.h>

#include <osmscout/Preprocess.h>

#include <osmscout/GenNodeDat.h>
#include <osmscout/GenWayDat.h>

#include <osmscout/NumericIndex.h>

#include <osmscout/GenAreaNodeIndex.h>
#include <osmscout/GenAreaWayIndex.h>
#include <osmscout/GenAreaWayIndex2.h>
#include <osmscout/GenNodeUseIndex.h>
#include <osmscout/GenCityStreetIndex.h>

static const size_t defaultStartStep=1;
static const size_t defaultEndStep=10;

ImportParameter::ImportParameter()
 : startStep(defaultStartStep),
   endStep(defaultEndStep),
   nodeIndexIntervalSize(50),
   numericIndexLevelSize(1024),
   areaAreaIndexMaxMag(14)
{
  // no code
}

std::string ImportParameter::GetMapfile() const
{
  return mapfile;
}

size_t ImportParameter::GetStartStep() const
{
  return startStep;
}

size_t ImportParameter::GetEndStep() const
{
  return endStep;
}

size_t ImportParameter::GetNodeIndexIntervalSize() const
{
  return nodeIndexIntervalSize;
}

size_t ImportParameter::GetNumericIndexLevelSize() const
{
  return numericIndexLevelSize;
}

size_t ImportParameter::GetAreaAreaIndexMaxMag() const
{
  return areaAreaIndexMaxMag;
}

void ImportParameter::SetMapfile(const std::string& mapfile)
{
  this->mapfile=mapfile;
}

void ImportParameter::SetStartStep(size_t startStep)
{
  this->startStep=startStep;
  this->endStep=defaultEndStep;
}

void ImportParameter::SetSteps(size_t startStep, size_t endStep)
{
  this->startStep=startStep;
  this->endStep=endStep;
}

void ImportParameter::SetNodeIndexIntervalSize(size_t nodeIndexIntervalSize)
{
  this->nodeIndexIntervalSize=nodeIndexIntervalSize;
}

bool Import(const ImportParameter& parameter,
            Progress& progress)
{
  // TODO: verify parameter

  TypeConfig typeConfig;
  size_t     startStep=parameter.GetStartStep();
  size_t     endStep=parameter.GetEndStep();

  progress.SetStep("Loading type config");

  if (!LoadTypeConfig("map.ost.xml",typeConfig)) {
    progress.Error("Cannot load type configuration!");
    return false;
  }

  if (startStep==1) {
    progress.SetStep("1 Preprocess");
    if (!Preprocess(typeConfig,
                    parameter,
                    progress)) {
      progress.Error("Cannot parse input file!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==2) {
    progress.SetStep("2 Generate 'nodes.dat'");
    if (!GenerateNodeDat(parameter,
                         progress)) {
      progress.Error("Cannot generate node data file!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==3) {
    progress.SetStep("3 Generate 'ways.dat'");
    if (!GenerateWayDat(typeConfig,
                        parameter,
                        progress)) {
      progress.Error("Cannot generate way data file!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==4) {
    progress.SetStep("4 Generating 'node.idx'");

    if (!GenerateNumericIndex<Id,Node>(parameter,
                                       progress,
                                       "nodes.dat",
                                       "node.idx")) {
      progress.Error("Cannot generate node2 index!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==5) {
    progress.SetStep("5 Generating 'areanode.idx'");

    if (!GenerateAreaNodeIndex(parameter,
                               progress)) {
      progress.Error("Cannot generate area node index!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==6) {
    progress.SetStep("6 Generating 'way.idx'");

    if (!GenerateNumericIndex<Id,Way>(parameter,
                                      progress,
                                      "ways.dat",
                                      "way.idx")) {
      progress.Error("Cannot generate way index!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==7) {
    progress.SetStep("7 Generating 'areaway.idx'");

    if (!GenerateAreaWayIndex(parameter,
                              progress)) {
      progress.Error("Cannot generate area way index!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==8) {
    progress.SetStep("8 Generating 'citystreet.idx'");

    if (!GenerateCityStreetIndex(typeConfig,
                                 parameter,
                                 progress)) {
      progress.Error("Cannot generate city street index!");
      return false;
    }

    startStep++;
  }

  if (startStep>endStep) {
    return true;
  }

  if (startStep==9) {
    progress.SetStep("9 Generating 'nodeuse.idx'");

    if (!GenerateNodeUseIndex(typeConfig,
                              parameter,
                              progress)) {
      progress.Error("Cannot generate node usage index!");
      return false;
    }

    startStep++;
  }

  if (startStep==10) {
    progress.SetStep("10 Generating 'areaway2.idx'");

    if (!GenerateAreaWayIndex2(parameter,
                               progress)) {
      progress.Error("Cannot generate area way index 2!");
      return false;
    }

    startStep++;
  }

  return true;
}

