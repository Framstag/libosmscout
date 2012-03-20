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

#include <cmath>
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

static std::wstring TimeToWString(double time)
{
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";

  time-=std::floor(time);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);

  stream << "h";

  return Lum::Base::StringToWString(stream.str());
}

std::wstring RouteDialog::RouteModelPainter::GetCellData() const
{
  const RouteStep step=dynamic_cast<const RouteModel*>(GetModel())->GetEntry(GetRow());

  if (GetColumn()==1) {
    return step.distance;
  }
  else if (GetColumn()==2) {
    return step.distanceDelta;
  }
  else if (GetColumn()==3) {
    return step.time;
  }
  else if (GetColumn()==4) {
    return step.timeDelta;
  }
  else if (GetColumn()==5) {
    return step.description;
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
  std::list<RouteStep>       routeSteps;
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
   routeModel(new RouteModel(result.routeSteps))
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
  headerModel->AddColumn(L"At",Lum::Base::Size::stdCharWidth,7);
  headerModel->AddColumn(L"After",Lum::Base::Size::stdCharWidth,7);
  headerModel->AddColumn(L"Time",Lum::Base::Size::stdCharWidth,6);
  headerModel->AddColumn(L"After",Lum::Base::Size::stdCharWidth,6);
  headerModel->AddColumn(L"Instruction",Lum::Base::Size::stdCharWidth,20,true);

  RouteModelPainter *routeModelPainter=new RouteModelPainter();
  routeModelPainter->SetAlignment(1, RouteModelPainter::right);
  routeModelPainter->SetAlignment(2, RouteModelPainter::right);
  routeModelPainter->SetAlignment(3, RouteModelPainter::left);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,60);
  table->SetHeight(Lum::Base::Size::stdCharHeight,10);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(routeModel);
  table->SetPainter(routeModelPainter);
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

bool RouteDialog::HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
{
  return node.HasDescription(osmscout::RouteDescription::NODE_START_DESC) ||
         node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC) ||
         node.HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
}

void RouteDialog::PrepareRouteStep(const std::list<osmscout::RouteDescription::Node>::const_iterator& prevNode,
                                   const std::list<osmscout::RouteDescription::Node>::const_iterator& node,
                                   size_t lineCount,
                                   RouteStep& step)
{
  if (lineCount==0) {
    step.distance=DistanceToWString(node->GetDistance());
    step.time=TimeToWString(node->GetTime());

    if (prevNode!=result.routeDescription.Nodes().end() && node->GetDistance()-prevNode->GetDistance()!=0.0) {
      step.distanceDelta=DistanceToWString(node->GetDistance()-prevNode->GetDistance());
    }
    if (prevNode!=result.routeDescription.Nodes().end() && node->GetTime()-prevNode->GetTime()!=0.0) {
      step.timeDelta=TimeToWString(node->GetTime()-prevNode->GetTime());
    }
  }
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
          result.startNode=way->nodes[0].GetId();

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
          result.endNode=way->nodes[0].GetId();

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
                                                       result.routeDescription,
                                                       Lum::Base::WStringToUTF8(result.start),
                                                       Lum::Base::WStringToUTF8(result.end));

    std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=result.routeDescription.Nodes().end();
    for (std::list<osmscout::RouteDescription::Node>::const_iterator node=result.routeDescription.Nodes().begin();
         node!=result.routeDescription.Nodes().end();
         ++node) {

      if (!HasRelevantDescriptions(*node)) {
        continue;
      }

      size_t lineCount=0;

      if (node->HasDescription(osmscout::RouteDescription::NODE_START_DESC)) {
        osmscout::RouteDescription::DescriptionRef   description=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
        osmscout::RouteDescription::StartDescription *startDescription=dynamic_cast<osmscout::RouteDescription::StartDescription*>(description.Get());
        RouteStep                                    step;

        PrepareRouteStep(prevNode,node,lineCount,step);

        step.description=L"Start at \"" +Lum::Base::UTF8ToWString(startDescription->GetDescription()) + L"\"";

        result.routeSteps.push_back(step);

        lineCount++;
      }
      if (node->HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
        osmscout::RouteDescription::DescriptionRef    description=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
        osmscout::RouteDescription::TargetDescription *targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(description.Get());
        RouteStep                                     step;

        PrepareRouteStep(prevNode,node,lineCount,step);
        step.description=L"Target reached \"" + Lum::Base::UTF8ToWString(targetDescription->GetDescription()) + L"\"";

        result.routeSteps.push_back(step);

        lineCount++;
      }
      if (node->HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC)) {
        osmscout::RouteDescription::DescriptionRef  description=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
        osmscout::RouteDescription::NameDescription *nameDescription=dynamic_cast<osmscout::RouteDescription::NameDescription*>(description.Get());
        RouteStep                                   step;

        PrepareRouteStep(prevNode,node,lineCount,step);

        step.description=L"Way ";


        if (!nameDescription->GetName().empty()) {
          step.description+=Lum::Base::UTF8ToWString(nameDescription->GetName());
        }

        if (!nameDescription->GetName().empty() &&
            !nameDescription->GetRef().empty()) {
          step.description+=L" (";
        }

        if (!nameDescription->GetRef().empty()) {
          step.description+=Lum::Base::UTF8ToWString(nameDescription->GetRef());
        }

        if (!nameDescription->GetName().empty() &&
            !nameDescription->GetRef().empty()) {
          step.description+=L")";
        }

        result.routeSteps.push_back(step);

        lineCount++;
      }

      prevNode=node;
    }

    routeModel->Notify();

    if (databaseTask->TransformRouteDataToWay(result.routeData,way)) {
      databaseTask->ClearRoute();
      databaseTask->AddRoute(way);
    }
    else {
      std::cerr << "Error while transforming route" << std::endl;
    }
  }
  else {
    Dialog::Resync(model,msg);
  }
}

