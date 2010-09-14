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

#include <osmscout/Route.h>

#include "DatabaseTask.h"

/**
 */
class RouteDialog : public Lum::Dlg::ActionDialog
{
private:
  typedef Lum::Model::StdRefTable<osmscout::RouteDescription::RouteStep,
          std::list<osmscout::RouteDescription::RouteStep> >             RouteModel;
  typedef Lum::Base::Reference<RouteModel>                               RouteModelRef;

  class RouteModelPainter : public Lum::StringCellPainter
  {
  public:
    std::wstring GetCellData() const;
  };

private:
  DatabaseTask*         databaseTask;
  Lum::Model::ActionRef routeAction;
  Lum::Model::StringRef start;
  bool                  hasStart;
  Lum::Model::ActionRef startAction;
  Lum::Model::StringRef end;
  bool                  hasEnd;
  Lum::Model::ActionRef endAction;
  RouteModelRef         routeModel;

public:
  RouteDialog(DatabaseTask* databaseTask);

  Lum::Object* GetContent();
  void GetActions(std::vector<Lum::Dlg::ActionInfo>& actions);
  void Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg);
};

#endif
