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

static std::wstring MoveToTurnCommand(osmscout::RouteDescription::DirectionDescription::Move move)
{
  switch (move) {
  case osmscout::RouteDescription::DirectionDescription::sharpLeft:
    return L"Turn sharp left";
  case osmscout::RouteDescription::DirectionDescription::left:
    return L"Turn left";
  case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
    return L"Turn slightly left";
  case osmscout::RouteDescription::DirectionDescription::straightOn:
    return L"Straight on";
  case osmscout::RouteDescription::DirectionDescription::slightlyRight:
    return L"Turn slightly right";
  case osmscout::RouteDescription::DirectionDescription::right:
    return L"Turn right";
  case osmscout::RouteDescription::DirectionDescription::sharpRight:
    return L"Turn sharp right";
  }

  assert(false);

  return L"???";
}

static std::string CrossingWaysDescriptionToString(const osmscout::RouteDescription::CrossingWaysDescription& crossingWaysDescription)
{
  std::set<std::string>                          names;
  osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription.GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription.GetTargetDesccription();

  if (originDescription.Valid()) {
    std::string nameString=originDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (targetDescription.Valid()) {
    std::string nameString=targetDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  for (std::list<osmscout::RouteDescription::NameDescriptionRef>::const_iterator name=crossingWaysDescription.GetDescriptions().begin();
      name!=crossingWaysDescription.GetDescriptions().end();
      ++name) {
    std::string nameString=(*name)->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (names.size()>0) {
    std::ostringstream stream;

    for (std::set<std::string>::const_iterator name=names.begin();
        name!=names.end();
        ++name) {
      if (name!=names.begin()) {
        stream << ", ";
      }
      stream << "'" << *name << "'";
    }

    return stream.str();
  }
  else {
    return "";
  }
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
  osmscout::ObjectFileRef    startObject;
  size_t                     startNodeIndex;
  std::wstring               end;
  osmscout::ObjectFileRef    endObject;
  size_t                     endNodeIndex;
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

void RouteDialog::DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                       const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep startAt;

  startAt.description=L"Start at '"+Lum::Base::UTF8ToWString(startDescription->GetDescription())+L"'";
  result.routeSteps.push_back(startAt);

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    RouteStep driveAlong;

    driveAlong.description=L"Drive along '"+Lum::Base::UTF8ToWString(nameDescription->GetDescription())+L"'";
    result.routeSteps.push_back(driveAlong);
  }
}

void RouteDialog::DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached;

  targetReached.description=L"Target reached '"+Lum::Base::UTF8ToWString(targetDescription->GetDescription())+L"'";
  result.routeSteps.push_back(targetReached);
}

void RouteDialog::DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
                                      const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                      const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                      const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep at;

    at.description=L"At crossing "+Lum::Base::UTF8ToWString(crossingWaysString);
    result.routeSteps.push_back(at);
  }

  RouteStep turn;

  if (directionDescription.Valid()) {
    turn.description=MoveToTurnCommand(directionDescription->GetCurve());
  }
  else {
    turn.description=L"Turn";
  }

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    turn.description+=L" into '"+Lum::Base::UTF8ToWString(nameDescription->GetDescription())+L"'";
  }

  result.routeSteps.push_back(turn);
}

void RouteDialog::DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                                 const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep   at;

    at.description=L"At crossing ";
    at.description+=Lum::Base::UTF8ToWString(crossingWaysString);

    result.routeSteps.push_back(at);
  }

  RouteStep enter;

  enter.description=L"Enter roundabout";

  result.routeSteps.push_back(enter);
}

void RouteDialog::DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                                 const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description=L"Leave roundabout (";
  leave.description+=Lum::Base::NumberToWString(roundaboutLeaveDescription->GetExitCount());
  leave.description+=L". exit)";

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    leave.description+=L" into street '";
    leave.description+=Lum::Base::UTF8ToWString(nameDescription->GetDescription());
    leave.description+=L"'";
  }

  result.routeSteps.push_back(leave);
}

void RouteDialog::DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                               const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep at;

    at.description=L"At crossing ";
    at.description+=Lum::Base::UTF8ToWString(crossingWaysString);
    result.routeSteps.push_back(at);
  }

  RouteStep enter;

  enter.description=L"Enter motorway";

  if (motorwayEnterDescription->GetToDescription().Valid() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {
    enter.description+=L" '";
    enter.description+=Lum::Base::UTF8ToWString(motorwayEnterDescription->GetToDescription()->GetDescription());
    enter.description+=L"'";
  }

  result.routeSteps.push_back(enter);
}

void RouteDialog::DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
{
  RouteStep change;

  change.description=L"Change motorway";

  if (motorwayChangeDescription->GetFromDescription().Valid() &&
      motorwayChangeDescription->GetFromDescription()->HasName()) {
    change.description+=L" from '";
    change.description+=Lum::Base::UTF8ToWString(motorwayChangeDescription->GetFromDescription()->GetDescription());
    change.description+=L"'";
  }

  if (motorwayChangeDescription->GetToDescription().Valid() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {
    change.description+=L" to '";
    change.description+=Lum::Base::UTF8ToWString(motorwayChangeDescription->GetToDescription()->GetDescription());
    change.description+=L"'";
  }

  result.routeSteps.push_back(change);
}

void RouteDialog::DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                               const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                               const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description=L"Leave motorway";

  if (motorwayLeaveDescription->GetFromDescription().Valid() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {
    leave.description+=L" '";
    leave.description+=Lum::Base::UTF8ToWString(motorwayLeaveDescription->GetFromDescription()->GetDescription());
    leave.description+=L"'";
  }

  if (directionDescription.Valid() &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyLeft &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::straightOn &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyRight) {
    leave.description+=MoveToTurnCommand(directionDescription->GetCurve());
  }

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    leave.description+=L" into '";
    leave.description+=Lum::Base::UTF8ToWString(nameDescription->GetDescription());
    leave.description+=L"'";
  }

  result.routeSteps.push_back(leave);
}

void RouteDialog::DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  RouteStep changed;

  changed.description=L"Way changes name";
  if (nameChangedDescription->GetOriginDesccription().Valid()) {
    changed.description+=L" from '";
    changed.description+=Lum::Base::UTF8ToWString(nameChangedDescription->GetOriginDesccription()->GetDescription());
    changed.description+=L"'";
  }

  changed.description+=L"' to '";
  changed.description+=Lum::Base::UTF8ToWString(nameChangedDescription->GetTargetDesccription()->GetDescription());
  changed.description+=L"'";

  result.routeSteps.push_back(changed);
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

      if (dialog->HasResult()) {
        LocationSearchDialog::Location location=dialog->GetResult();

        switch (location.object.GetType()) {
        case osmscout::refArea:
        case osmscout::refWay:
          result.startObject=location.object;
          result.startNodeIndex=0;
          result.start=location.label;

          start->Set(result.start);

          hasResult=true;
          break;
        default:
          // TODO: Open some error dialog
          hasResult=false;
          result.start.clear();
          start->Set(L"");
          break;
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
        LocationSearchDialog::Location location=dialog->GetResult();
        osmscout::WayRef               way;

        switch (location.object.GetType()) {
        case osmscout::refArea:
        case osmscout::refWay:
          result.endObject=location.object;
          result.endNodeIndex=0;
          result.end=location.label;

          end->Set(result.end);

          hasResult=true;
          break;
        default:
          // TODO: Open some error dialog
          hasResult=false;
          result.end.clear();
          end->Set(L"");
          break;
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

    result.routeSteps.clear();
    routeModel->Notify();

    osmscout::Way way;

    if (!databaseTask->CalculateRoute(result.startObject,
                                      result.startNodeIndex,
                                      result.endObject,
                                      result.endNodeIndex,
                                      result.routeData)) {
      std::cerr << "There was an error while routing!" << std::endl;
      return;
    }

    //databaseTask->DumpStatistics();

    databaseTask->TransformRouteDataToRouteDescription(result.routeData,
                                                       result.routeDescription,
                                                       Lum::Base::WStringToUTF8(result.start),
                                                       Lum::Base::WStringToUTF8(result.end));

    size_t                         roundaboutCrossingCounter=0;
    std::list<RouteStep>::iterator lastStep=result.routeSteps.end();

    std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=result.routeDescription.Nodes().end();
    for (std::list<osmscout::RouteDescription::Node>::const_iterator node=result.routeDescription.Nodes().begin();
         node!=result.routeDescription.Nodes().end();
         ++node) {
      osmscout::RouteDescription::DescriptionRef                 desc;
      osmscout::RouteDescription::NameDescriptionRef             nameDescription;
      osmscout::RouteDescription::DirectionDescriptionRef        directionDescription;
      osmscout::RouteDescription::NameChangedDescriptionRef      nameChangedDescription;
      osmscout::RouteDescription::CrossingWaysDescriptionRef     crossingWaysDescription;

      osmscout::RouteDescription::StartDescriptionRef            startDescription;
      osmscout::RouteDescription::TargetDescriptionRef           targetDescription;
      osmscout::RouteDescription::TurnDescriptionRef             turnDescription;
      osmscout::RouteDescription::RoundaboutEnterDescriptionRef  roundaboutEnterDescription;
      osmscout::RouteDescription::RoundaboutLeaveDescriptionRef  roundaboutLeaveDescription;
      osmscout::RouteDescription::MotorwayEnterDescriptionRef    motorwayEnterDescription;
      osmscout::RouteDescription::MotorwayChangeDescriptionRef   motorwayChangeDescription;
      osmscout::RouteDescription::MotorwayLeaveDescriptionRef    motorwayLeaveDescription;

      desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
      if (desc.Valid()) {
        nameDescription=dynamic_cast<osmscout::RouteDescription::NameDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::DIRECTION_DESC);
      if (desc.Valid()) {
        directionDescription=dynamic_cast<osmscout::RouteDescription::DirectionDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
      if (desc.Valid()) {
        nameChangedDescription=dynamic_cast<osmscout::RouteDescription::NameChangedDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
      if (desc.Valid()) {
        crossingWaysDescription=dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
      if (desc.Valid()) {
        startDescription=dynamic_cast<osmscout::RouteDescription::StartDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
      if (desc.Valid()) {
        targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::TURN_DESC);
      if (desc.Valid()) {
        turnDescription=dynamic_cast<osmscout::RouteDescription::TurnDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC);
      if (desc.Valid()) {
        roundaboutEnterDescription=dynamic_cast<osmscout::RouteDescription::RoundaboutEnterDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC);
      if (desc.Valid()) {
        roundaboutLeaveDescription=dynamic_cast<osmscout::RouteDescription::RoundaboutLeaveDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC);
      if (desc.Valid()) {
        motorwayEnterDescription=dynamic_cast<osmscout::RouteDescription::MotorwayEnterDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC);
      if (desc.Valid()) {
        motorwayChangeDescription=dynamic_cast<osmscout::RouteDescription::MotorwayChangeDescription*>(desc.Get());
      }

      desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC);
      if (desc.Valid()) {
        motorwayLeaveDescription=dynamic_cast<osmscout::RouteDescription::MotorwayLeaveDescription*>(desc.Get());
      }

      if (crossingWaysDescription.Valid() &&
          roundaboutCrossingCounter>0 &&
          crossingWaysDescription->GetExitCount()>1) {
        roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
      }

      if (startDescription.Valid()) {
        DumpStartDescription(startDescription,
                             nameDescription);
      }
      else if (targetDescription.Valid()) {
        DumpTargetDescription(targetDescription);
      }
      else if (turnDescription.Valid()) {
        DumpTurnDescription(turnDescription,
                            crossingWaysDescription,
                            directionDescription,
                            nameDescription);
      }
      else if (roundaboutEnterDescription.Valid()) {
        DumpRoundaboutEnterDescription(roundaboutEnterDescription,
                                       crossingWaysDescription);

        roundaboutCrossingCounter=1;
      }
      else if (roundaboutLeaveDescription.Valid()) {
        DumpRoundaboutLeaveDescription(roundaboutLeaveDescription,
                                       nameDescription);

        roundaboutCrossingCounter=0;
      }
      else if (motorwayEnterDescription.Valid()) {
        DumpMotorwayEnterDescription(motorwayEnterDescription,
                                     crossingWaysDescription);
      }
      else if (motorwayChangeDescription.Valid()) {
        DumpMotorwayChangeDescription(motorwayChangeDescription);
      }
      else if (motorwayLeaveDescription.Valid()) {
        DumpMotorwayLeaveDescription(motorwayLeaveDescription,
                                     directionDescription,
                                     nameDescription);
      }
      else if (nameChangedDescription.Valid()) {
        DumpNameChangedDescription(nameChangedDescription);
      }
      else {
        continue;
      }

      std::list<RouteStep>::iterator current;

      if (lastStep==result.routeSteps.end()) {
        current=result.routeSteps.begin();
      }
      else {
        current=lastStep;

        current++;
      }

      if (current!=result.routeSteps.end()) {
        current->distance=DistanceToWString(node->GetDistance());
        current->time=TimeToWString(node->GetTime());

        if (prevNode!=result.routeDescription.Nodes().end() &&
            node->GetDistance()-prevNode->GetDistance()!=0.0) {
          current->distanceDelta=DistanceToWString(node->GetDistance()-prevNode->GetDistance());
        }

        if (prevNode!=result.routeDescription.Nodes().end() &&
            node->GetTime()-prevNode->GetTime()!=0.0) {
          current->timeDelta=TimeToWString(node->GetTime()-prevNode->GetTime());
        }
      }

      while (current!=result.routeSteps.end()) {
        lastStep++;
        current++;
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

