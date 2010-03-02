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

#include <Lum/Label.h>
#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/Database.h>

#include "CityStreetSearchDialog.h"

bool RouteDialog::RouteSelection::IsStartValid() const
{
  return !startCity.empty() && !startStreet.empty() && startWay!=0 && startNode!=0;
}

bool RouteDialog::RouteSelection::IsEndValid() const
{
  return !endCity.empty() && !endStreet.empty() && endWay!=0 && endNode!=0;
}

RouteDialog::RouteDialog(DatabaseTask* databaseTask,
                         const RouteSelection& selection)
 : databaseTask(databaseTask),
   okAction(new Lum::Model::Action()),
   start(new Lum::Model::String()),
   hasStart(false),
   startAction(new Lum::Model::Action()),
   end(new Lum::Model::String()),
   hasEnd(false),
   endAction(new Lum::Model::Action()),
   result(selection),
   hasResult(false)
{
  Observe(okAction);
  Observe(GetClosedAction());
  Observe(startAction);
  Observe(endAction);

  start->Disable();
  end->Disable();
  okAction->Disable();
  /*
  if (result.startCity.empty()) {
    result.startCity="Bonn";
    result.startWay=14332719;
    result.startNode=138190834;
    result.startStreet="Promenadenweg";
    start->Set(L"Bonn, Promenadenweg");
    hasStart=true;

    result.endCity="Dortmund";
    result.endStreet="Am Birkenbaum";
    result.endWay=10414977;
    result.endNode=283372120;
    end->Set(L"Dortmund, Am Birkenbaum");
    hasEnd=true;
  }

  if (result.IsStartValid()) {
    start->Set(Lum::Base::UTF8ToWString(result.startCity)+
               L", "+
               Lum::Base::UTF8ToWString(result.startStreet));
    hasStart=true;
  }

  if (result.IsEndValid()) {
    end->Set(Lum::Base::UTF8ToWString(result.endCity)+
             L", "+
             Lum::Base::UTF8ToWString(result.endStreet));
    hasEnd=true;
  }

  if (result.IsStartValid() && result.IsEndValid()) {
    okAction->Enable();
  } */
}

Lum::Object* RouteDialog::GetContent()
{
  Lum::Label            *label;
  Lum::Panel            *panel,*sub;
  Lum::Model::HeaderRef headerModel;

  panel=Lum::VPanel::Create(true,true);

  label=Lum::Label::Create(true,false);

  sub=Lum::HPanel::Create(true,false);
  sub->Add(Lum::String::Create(start,30,true,false));
  sub->Add(Lum::Button::Create(L"...",startAction));
  label->AddLabel(L"Start:",sub);

  sub=Lum::HPanel::Create(true,false);
  sub->Add(Lum::String::Create(end,30,true,false));
  sub->Add(Lum::Button::Create(L"...",endAction));
  label->AddLabel(L"End:",sub);

  panel->Add(label);

  return panel;
}

void RouteDialog::GetActions(std::vector<Lum::Dlg::ActionInfo>& actions)
{
  Lum::Dlg::ActionDialog::CreateActionInfosOkCancel(actions,okAction,GetClosedAction());
}

void RouteDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==startAction && startAction->IsEnabled() && startAction->IsFinished()) {
    bool hasResult=false;

    CityStreetSearchDialog *dialog;

    dialog=new CityStreetSearchDialog(databaseTask);
    dialog->SetParent(this);
    if (dialog->Open()) {
      dialog->EventLoop();
      dialog->Close();

      if (dialog->HasResult()) {
        osmscout::AdminRegion region=dialog->GetResultAdminRegion();
        osmscout::Location    location=dialog->GetResultLocation();
        osmscout::Way         way;

        assert(location.references.front().GetType()==osmscout::refWay);

        // TODO: Check that this is a way!
        result.startCity=region.name;
        result.startStreet=location.name;
        result.startWay=location.references.front().GetId();

        if (databaseTask->GetWay(result.startWay,way)) {
          result.startNode=way.nodes[0].id;

          hasResult=true;
        }
      }
    }

    delete dialog;

    if (hasResult) {
      hasStart=true;

      start->Set(Lum::Base::UTF8ToWString(result.startCity)+
                 L", "+
                 Lum::Base::UTF8ToWString(result.startStreet));

      if (hasStart && hasEnd) {
        okAction->Enable();
      }
    }
  }
  else if (model==endAction && endAction->IsEnabled() && endAction->IsFinished()) {
    bool hasResult=false;

    CityStreetSearchDialog *dialog;

    dialog=new CityStreetSearchDialog(databaseTask);
    dialog->SetParent(this);
    if (dialog->Open()) {
      dialog->EventLoop();
      dialog->Close();

      if (dialog->HasResult()) {
        osmscout::AdminRegion region=dialog->GetResultAdminRegion();
        osmscout::Location    location=dialog->GetResultLocation();
        osmscout::Way         way;

        assert(location.references.front().GetType()==osmscout::refWay);

        result.endCity=region.name;
        result.endStreet=location.name;
        result.endWay=location.references.front().GetId();

        if (databaseTask->GetWay(result.endWay,way)) {
          result.endNode=way.nodes[0].id;

          hasResult=true;
        }
      }
    }

    delete dialog;

    if (hasResult) {
      hasEnd=true;

      end->Set(Lum::Base::UTF8ToWString(result.endCity)+
               L", "+
               Lum::Base::UTF8ToWString(result.endStreet));

      if (hasStart && hasEnd) {
        okAction->Enable();
      }
    }
  }
  else if (model==okAction && okAction->IsEnabled() && okAction->IsFinished()) {
    assert(hasStart && hasEnd);

    osmscout::RouteData        routeData;
    osmscout::RouteDescription routeDescription;
    osmscout::Way              way;

    if (!databaseTask->CalculateRoute(result.startWay,result.startNode,
                                      result.endWay,result.endNode,
                                      routeData)) {
      std::cerr << "There was an error while routing!" << std::endl;
      hasResult=true;
      Exit();
      return;
    }

    //databaseTask->DumpStatistics();

    databaseTask->TransformRouteDataToRouteDescription(routeData,routeDescription);
    double lastDistance = 0;

    for (std::list<osmscout::RouteDescription::RouteStep>::const_iterator step=routeDescription.Steps().begin();
         step!=routeDescription.Steps().end();
         ++step) {
#if defined(HTML)
      std::cout << "<tr><td>";
#endif
      std::cout.setf(std::ios::right);
      std::cout.fill(' ');
      std::cout.width(5);
      std::cout.setf(std::ios::fixed);
      std::cout.precision(1);
      std::cout << step->GetDistance() << "km ";

      if (step->GetDistance()-lastDistance!=0.0) {
        std::cout.setf(std::ios::right);
        std::cout.fill(' ');
        std::cout.width(5);
        std::cout.setf(std::ios::fixed);
        std::cout.precision(1);
        std::cout << step->GetDistance()-lastDistance << "km ";
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

      lastDistance=step->GetDistance();
    }

    std::cout << std::setprecision(6); // back to default

    databaseTask->TransformRouteDataToWay(routeData,way);
    databaseTask->ClearRoute();
    databaseTask->AddRoute(way);

    hasResult=true;
    Exit();
  }
  else {
    Dialog::Resync(model,msg);
  }
}

bool RouteDialog::HasResult() const
{
  return hasResult;
}

const RouteDialog::RouteSelection& RouteDialog::GetResult() const
{
  return result;
}

