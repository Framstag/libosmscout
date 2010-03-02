#ifndef ROUTEDIALOG_H
#define ROUTEDIALOG_H

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

#include <Lum/Dlg/ActionDialog.h>

#include <Lum/Model/String.h>
#include <Lum/Model/Table.h>

#include <Lum/Object.h>

#include "DatabaseTask.h"

/**
 */
class RouteDialog : public Lum::Dlg::ActionDialog
{
public:
  struct RouteSelection
  {
    std::string  startCity;
    std::string  startStreet;
    osmscout::Id startWay;
    osmscout::Id startNode;
    std::string  endCity;
    std::string  endStreet;
    osmscout::Id endWay;
    osmscout::Id endNode;

    bool IsStartValid() const;
    bool IsEndValid() const;
  };

private:
  DatabaseTask*         databaseTask;
  Lum::Model::ActionRef okAction;
  Lum::Model::StringRef start;
  bool                  hasStart;
  Lum::Model::ActionRef startAction;
  Lum::Model::StringRef end;
  bool                  hasEnd;
  Lum::Model::ActionRef endAction;
  RouteSelection        result;
  bool                  hasResult;

public:
  RouteDialog(DatabaseTask* databaseTask,
              const RouteSelection& selection);

  Lum::Object* GetContent();
  void GetActions(std::vector<Lum::Dlg::ActionInfo>& actions);
  void Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg);

  bool HasResult() const;
  const RouteSelection& GetResult() const;
};

#endif
