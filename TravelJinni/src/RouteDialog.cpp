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

#include "RouteDialog.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include <Lum/Label.h>
#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/Database.h>

#include "LocationSearchDialog.h"

static std::wstring DistanceToWString(double distance)
{
  std::ostringstream stream;

  stream.setf(std::ios::fixed);
  stream.precision(1);
  stream << distance << "km";

  return Lum::Base::StringToWString(stream.str());
}

std::wstring RouteDialog::RouteModelPainter::GetCellData() const
{
  const osmscout::RouteDescription::RouteStep step=dynamic_cast<const RouteModel*>(GetModel())->GetEntry(GetRow());

  if (GetColumn()==1) {
    return DistanceToWString(step.GetAt());
  }
  else if (GetColumn()==2) {
    return DistanceToWString(step.GetAfter());
  }
  else if (GetColumn()==3) {
    std::wstring action;

    switch (step.GetAction()) {
    case osmscout::RouteDescription::start:
      action=L"Start at ";
      if (!step.GetName().empty()) {
        action.append(Lum::Base::UTF8ToWString(step.GetName()));

        if (!step.GetRefName().empty()) {
          action.append(std::wstring(L" (")+Lum::Base::UTF8ToWString(step.GetRefName())+L")");
        }
      }
      else {
        action.append(Lum::Base::UTF8ToWString(step.GetRefName()));
      }
      break;
    case osmscout::RouteDescription::drive:
      action=L"drive along ";
      if (!step.GetName().empty()) {
        action.append(Lum::Base::UTF8ToWString(step.GetName()));

        if (!step.GetRefName().empty()) {
          action.append(std::wstring(L" (")+Lum::Base::UTF8ToWString(step.GetRefName())+L")");
        }
      }
      else {
        action.append(Lum::Base::UTF8ToWString(step.GetRefName()));
      }
      break;
    case osmscout::RouteDescription::switchRoad:
      action=L"turn into ";
      if (!step.GetName().empty()) {
        action.append(Lum::Base::UTF8ToWString(step.GetName()));

        if (!step.GetRefName().empty()) {
          action.append(std::wstring(L" (")+Lum::Base::UTF8ToWString(step.GetRefName())+L")");
        }
      }
      else {
        action.append(Lum::Base::UTF8ToWString(step.GetRefName()));
      }
      break;
    case osmscout::RouteDescription::reachTarget:
      action=L"Arriving at ";
      if (!step.GetName().empty()) {
        action.append(Lum::Base::UTF8ToWString(step.GetName()));

        if (!step.GetRefName().empty()) {
          action.append(std::wstring(L" (")+Lum::Base::UTF8ToWString(step.GetRefName())+L")");
        }
      }
      else {
        action.append(Lum::Base::UTF8ToWString(step.GetRefName()));
      }
      break;
    case osmscout::RouteDescription::pass:
      action=L"passing along ";
      if (!step.GetName().empty()) {
        action.append(Lum::Base::UTF8ToWString(step.GetName()));

        if (!step.GetRefName().empty()) {
          action.append(std::wstring(L" (")+Lum::Base::UTF8ToWString(step.GetRefName())+L")");
        }
      }
      else {
        action.append(Lum::Base::UTF8ToWString(step.GetRefName()));
      }
      break;
    }
    return action;
  }

  return L"???";
}

struct RouteSelection
{
  std::wstring               start;
  osmscout::Id               startWay;
  osmscout::Id               startNode;
  std::wstring               end;
  osmscout::Id               endWay;
  osmscout::Id               endNode;
  osmscout::RouteData        routeData;
  osmscout::RouteDescription routeDescription;
};

RouteSelection result;

RouteDialog::RouteDialog(DatabaseTask* databaseTask)
 : databaseTask(databaseTask),
   routeAction(new Lum::Model::Action()),
   start(new Lum::Model::String()),
   hasStart(false),
   startAction(new Lum::Model::Action()),
   end(new Lum::Model::String()),
   hasEnd(false),
   endAction(new Lum::Model::Action()),
   routeModel(new RouteModel(result.routeDescription.Steps()))
{
  Observe(routeAction);
  Observe(GetClosedAction());
  Observe(startAction);
  Observe(endAction);

  start->Disable();
  end->Disable();
  routeAction->Disable();

  if (!result.start.empty()) {
    start->Set(result.start);
    hasStart=true;
  }

  if (!result.end.empty()) {
    end->Set(result.end);
    hasEnd=true;
  }

  if (hasStart && hasEnd) {
    routeAction->Enable();
  }
}

Lum::Object* RouteDialog::GetContent()
{
  Lum::Label            *label;
  Lum::Panel            *panel,*sub;
  Lum::Table            *table;
  Lum::Model::HeaderRef headerModel;

  panel=Lum::VPanel::Create(true,true);

  label=Lum::Label::Create(true,false);

  sub=Lum::HPanel::Create(true,false);
  sub->Add(Lum::String::Create(start,60,true,false));
  sub->Add(Lum::Button::Create(L"...",startAction));
  label->AddLabel(L"Start:",sub);

  sub=Lum::HPanel::Create(true,false);
  sub->Add(Lum::String::Create(end,60,true,false));
  sub->Add(Lum::Button::Create(L"...",endAction));
  label->AddLabel(L"End:",sub);

  panel->Add(label);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"At",Lum::Base::Size::stdCharWidth,8);
  headerModel->AddColumn(L"After",Lum::Base::Size::stdCharWidth,8);
  headerModel->AddColumn(L"Instruction",Lum::Base::Size::stdCharWidth,20,true);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,60);
  table->SetHeight(Lum::Base::Size::stdCharHeight,10);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(routeModel);
  table->SetPainter(new RouteModelPainter());
  table->SetHeaderModel(headerModel);
  panel->Add(table);

  //locationsModel->SetEmptyText(L"- no search criteria -");

  return panel;
}

void RouteDialog::GetActions(std::vector<Lum::Dlg::ActionInfo>& actions)
{
  actions.push_back(Lum::Dlg::ActionInfo(Lum::Dlg::ActionInfo::typeDefault,
                                    L"Route",
                                    routeAction,
                                    true));

  Lum::Dlg::ActionDialog::CreateActionInfosClose(actions,GetClosedAction());
}

void RouteDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==startAction && startAction->IsEnabled() && startAction->IsFinished()) {
    bool hasResult=false;

    LocationSearchDialog *dialog;

    dialog=new LocationSearchDialog(databaseTask);
    dialog->SetParent(this);
    if (dialog->Open()) {
      dialog->EventLoop();
      dialog->Close();

      // TODO: Check that this is a way!

      if (dialog->HasResult()) {
        osmscout::Location    location=dialog->GetResult();
        osmscout::WayRef      way;

        assert(location.references.front().GetType()==osmscout::refWay);

       result.startWay=location.references.front().GetId();

        if (databaseTask->GetWay(result.startWay,way)) {
          result.startNode=way->nodes[0].id;

          if (location.path.empty()) {
            result.start=Lum::Base::UTF8ToWString(location.name);
          }
          else {
            result.start=Lum::Base::UTF8ToWString(location.name)+
                         L" ("+Lum::Base::UTF8ToWString(osmscout::StringListToString(location.path))+L")";
          }

          start->Set(result.start);

          hasResult=true;
        }
        else {
          result.start.clear();
          start->Set(L"");
        }
      }
    }

    delete dialog;

    if (hasResult) {
      hasStart=true;
      if (hasStart && hasEnd) {
        routeAction->Enable();
      }
    }
  }
  else if (model==endAction && endAction->IsEnabled() && endAction->IsFinished()) {
    bool hasResult=false;

    LocationSearchDialog *dialog;

    dialog=new LocationSearchDialog(databaseTask);
    dialog->SetParent(this);
    if (dialog->Open()) {
      dialog->EventLoop();
      dialog->Close();

      // TODO: Check that this is a way!

      if (dialog->HasResult()) {
        osmscout::Location    location=dialog->GetResult();
        osmscout::WayRef      way;

        assert(location.references.front().GetType()==osmscout::refWay);

        result.endWay=location.references.front().GetId();

        if (databaseTask->GetWay(result.endWay,way)) {
          result.endNode=way->nodes[0].id;

          if (location.path.empty()) {
            result.end=Lum::Base::UTF8ToWString(location.name);
          }
          else {
            result.end=Lum::Base::UTF8ToWString(location.name)+
                       L" ("+Lum::Base::UTF8ToWString(osmscout::StringListToString(location.path))+L")";
          }

          end->Set(result.end);

          hasResult=true;
        }
        else {
          result.end.clear();
          end->Set(L"");
        }
      }
    }

    delete dialog;

    if (hasResult) {
      hasEnd=true;

      if (hasStart && hasEnd) {
        routeAction->Enable();
      }
    }
  }
  else if (model==routeAction &&
           routeAction->IsEnabled() &&
           routeAction->IsFinished()) {
    assert(hasStart && hasEnd);

    osmscout::Way way;

    if (!databaseTask->CalculateRoute(result.startWay,result.startNode,
                                      result.endWay,result.endNode,
                                      result.routeData)) {
      std::cerr << "There was an error while routing!" << std::endl;
      return;
    }

    //databaseTask->DumpStatistics();

    databaseTask->TransformRouteDataToRouteDescription(result.routeData,
                                                       result.routeDescription);

    routeModel->Notify();

    for (std::list<osmscout::RouteDescription::RouteStep>::const_iterator step=result.routeDescription.Steps().begin();
         step!=result.routeDescription.Steps().end();
         ++step) {
#if defined(HTML)
      std::cout << "<tr><td>";
#endif
      std::cout.setf(std::ios::right);
      std::cout.fill(' ');
      std::cout.width(5);
      std::cout.setf(std::ios::fixed);
      std::cout.precision(1);
      std::cout << step->GetAt() << "km ";

      if (step->GetAfter()!=0.0) {
        std::cout.setf(std::ios::right);
        std::cout.fill(' ');
        std::cout.width(5);
        std::cout.setf(std::ios::fixed);
        std::cout.precision(1);
        std::cout << step->GetAfter() << "km ";
      }
      else {
        std::cout << "        ";
      }

#if defined(HTML)
      std::cout <<"</td>";
#endif

#if defined(HTML)
      std::cout << "<td>";
#endif
      switch (step->GetAction()) {
      case osmscout::RouteDescription::start:
        std::cout << "Start at ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case osmscout::RouteDescription::drive:
        std::cout << "drive along ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case osmscout::RouteDescription::switchRoad:
        std::cout << "turn into ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case osmscout::RouteDescription::reachTarget:
        std::cout << "Arriving at ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case osmscout::RouteDescription::pass:
        std::cout << "passing along ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      }

#if defined(HTML)
      std::cout << "</td></tr>";
#endif
      std::cout << std::endl;
    }

    std::cout << std::setprecision(6); // back to default

    databaseTask->TransformRouteDataToWay(result.routeData,way);
    databaseTask->ClearRoute();
    databaseTask->AddRoute(way);
  }
  else {
    Dialog::Resync(model,msg);
  }
}

