/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings

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

#include "RoutingDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "SearchLocationDialog.h"

#include <cmath>
#include <iostream>
#include <iomanip>

static QString DistanceToString(double distance)
{
  std::ostringstream stream;

  stream.setf(std::ios::fixed);
  stream.precision(1);
  stream << distance << "km";

  return stream.str().c_str();
}

static QString TimeToString(double time)
{
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";

  time-=std::floor(time);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);

  stream << "h";

  return stream.str().c_str();
}

static QString MoveToTurnCommand(osmscout::RouteDescription::DirectionDescription::Move move)
{
  switch (move) {
  case osmscout::RouteDescription::DirectionDescription::sharpLeft:
    return "Turn sharp left";
  case osmscout::RouteDescription::DirectionDescription::left:
    return "Turn left";
  case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
    return "Turn slightly left";
  case osmscout::RouteDescription::DirectionDescription::straightOn:
    return "Straight on";
  case osmscout::RouteDescription::DirectionDescription::slightlyRight:
    return "Turn slightly right";
  case osmscout::RouteDescription::DirectionDescription::right:
    return "Turn right";
  case osmscout::RouteDescription::DirectionDescription::sharpRight:
    return "Turn sharp right";
  }

  assert(false);

  return "???";
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

struct RouteSelection
{
  QString                    start;
  osmscout::ObjectFileRef    startObject;
  size_t                     startNodeIndex;
  QString                    end;
  osmscout::ObjectFileRef    endObject;
  size_t                     endNodeIndex;
  osmscout::RouteData        routeData;
  osmscout::RouteDescription routeDescription;
  std::list<RouteStep>       routeSteps;
};

RouteSelection route;

RouteModel::RouteModel(QObject *parent)
 :QAbstractTableModel(parent)
{
  // no code
}

int RouteModel::rowCount(const QModelIndex &parent) const
{
  return route.routeSteps.size();
}

int RouteModel::columnCount(const QModelIndex &parent) const
{
  return 5;
}

QVariant RouteModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()) {
    return QVariant();
  }

  if (index.row()<0 || index.row()>=(int)route.routeDescription.Nodes().size()) {
    return QVariant();
  }

  if (role==Qt::DisplayRole) {
    std::list<RouteStep>::const_iterator step=route.routeSteps.begin();

    std::advance(step,index.row());

    if (index.column()==0) {
      return step->distance;
    }
    else if (index.column()==1) {
      return step->distanceDelta;
    }
    if (index.column()==2) {
      return step->time;
    }
    else if (index.column()==3) {
      return step->timeDelta;
    }
    else if (index.column()==4) {
      return step->description;
    }
  }
  else if (role==Qt::TextAlignmentRole) {
    if (index.column()==0  || index.column()==1) {
      return int(Qt::AlignRight|Qt::AlignVCenter);
    }
    else {
      return int(Qt::AlignLeft|Qt::AlignVCenter);
    }
  }

  return QVariant();
}

QVariant RouteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role==Qt::DisplayRole && orientation==Qt::Horizontal) {
    switch (section) {
    case 0:
      return "At";
    case 1:
      return "After";
    case 2:
      return "Time";
    case 3:
      return "After";
    case 4:
      return "Instruction";
    default:
      return QVariant();
    }
  }

  return QVariant();
}

void RouteModel::refresh()
{
  beginResetModel();
  endResetModel();
}

RoutingDialog::RoutingDialog(QWidget* parentWindow,
                             DBThread* dbThread)
 : QDialog(parentWindow,Qt::Dialog),
   dbThread(dbThread),
   from(new QLineEdit()),
   hasStart(false),
   to(new QLineEdit()),
   hasEnd(false),
   routeView(new QTableView()),
   routeModel(new RouteModel()),
   routeButton(new QPushButton("&Route"))
{
  QVBoxLayout *mainLayout=new QVBoxLayout();
  QFormLayout *formLayout=new QFormLayout();
  QHBoxLayout *fromBoxLayout=new QHBoxLayout();
  QHBoxLayout *toBoxLayout=new QHBoxLayout();
  QPushButton *selectFromButton=new QPushButton("...");
  QPushButton *selectToButton=new QPushButton("...");

  from->setReadOnly(true);
  from->setEnabled(false);
  from->setMinimumWidth(QApplication::fontMetrics().width("mlxi")/4*70);
  //from->setPlaceholderText("Starting location");
  to->setReadOnly(true);
  to->setEnabled(false);
  to->setMinimumWidth(QApplication::fontMetrics().width("mlxi")/4*70);
  //to->setPlaceholderText("Destination location");

  selectFromButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Fixed));
  selectToButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Fixed));

  fromBoxLayout->addWidget(from,0);
  fromBoxLayout->addWidget(selectFromButton,0);

  formLayout->addRow("From:",fromBoxLayout);

  toBoxLayout->addWidget(to,0);
  toBoxLayout->addWidget(selectToButton,0);

  formLayout->addRow("To:",toBoxLayout);

  mainLayout->addLayout(formLayout);

  routeView->setModel(routeModel);
  routeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  routeView->setSelectionMode(QAbstractItemView::SingleSelection);
  routeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  routeView->setShowGrid(false);
  routeView->setMinimumWidth(QApplication::fontMetrics().width("mlxi")/4*70);
  routeView->horizontalHeader()->setStretchLastSection(true);
  routeView->horizontalHeader()->setHighlightSections (false);
  routeView->verticalHeader()->hide();
  routeView->setColumnWidth(0,QApplication::fontMetrics().width("mlxi")/4*12);
  routeView->setColumnWidth(1,QApplication::fontMetrics().width("mlxi")/4*12);
  routeView->setColumnWidth(2,QApplication::fontMetrics().width("mlxi")/4*10);
  routeView->setColumnWidth(3,QApplication::fontMetrics().width("mlxi")/4*10);

  mainLayout->addWidget(routeView);

  QDialogButtonBox *buttonBox=new QDialogButtonBox();

  buttonBox->addButton(routeButton,QDialogButtonBox::ApplyRole);
  buttonBox->addButton(QDialogButtonBox::Close);

  routeButton->setEnabled(false);

  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  connect(selectFromButton,SIGNAL(clicked()),this,SLOT(SelectFrom()));
  connect(selectToButton,SIGNAL(clicked()),this,SLOT(SelectTo()));
  connect(routeButton,SIGNAL(clicked()),this,SLOT(Route()));
  connect(buttonBox->button(QDialogButtonBox::Close),SIGNAL(clicked()),this,SLOT(reject()));

  if (!route.start.isEmpty()) {
    from->setText(route.start);
    hasStart=true;
  }

  if (!route.end.isEmpty()) {
    to->setText(route.end);
    hasEnd=true;
  }

  if (hasStart && hasEnd) {
    routeButton->setEnabled(true);
  }
}

RoutingDialog::~RoutingDialog()
{
  // no code
}

void RoutingDialog::SelectFrom()
{
  std::cout << "Select from..." << std::endl;

  SearchLocationDialog dialog(this,
                              dbThread);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location location;
    std::string        label;
    osmscout::WayRef   way;

    location=dialog.GetLocationResult();

    switch (location.references.front().GetType()) {
    case osmscout::refArea:
    case osmscout::refWay:
      route.startObject=location.references.front();
      route.startNodeIndex=0;

      if (location.path.empty()) {
        route.start=QString::fromUtf8(location.name.c_str());
      }
      else {
        route.start=QString::fromUtf8(location.name.c_str())+
                     " ("+QString::fromUtf8(osmscout::StringListToString(location.path).c_str())+")";
      }

      from->setText(route.start);
       // Make sure, start of text is visible
      from->setCursorPosition(0);

      hasStart=true;

      break;
    default:
      // TODO: Open some error dialog
      hasStart=false;
      route.start.clear();
      from->setText("");
      break;
    }

    routeButton->setEnabled(hasStart && hasEnd);
  }
}

void RoutingDialog::SelectTo()
{
  std::cout << "Select to..." << std::endl;

  SearchLocationDialog dialog(this,
                              dbThread);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location   location;
    std::string          label;
    osmscout::WayRef     way;

    location=dialog.GetLocationResult();

    switch (location.references.front().GetType()) {
    case osmscout::refArea:
    case osmscout::refWay:
      route.endObject=location.references.front();
      route.endNodeIndex=0;

      if (location.path.empty()) {
        route.end=QString::fromUtf8(location.name.c_str());
      }
      else {
        route.end=QString::fromUtf8(location.name.c_str())+
                     " ("+QString::fromUtf8(osmscout::StringListToString(location.path).c_str())+")";
      }

      to->setText(route.end);
       // Make sure, start of text is visible
      to->setCursorPosition(0);

      hasEnd=true;
      break;
    default:
      // TODO: Open some error dialog
      hasEnd=false;
      route.end.clear();
      to->setText("");
      break;
    }

    routeButton->setEnabled(hasStart && hasEnd);
  }
}

void RoutingDialog::DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                         const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep startAt;

  startAt.description="Start at '"+QString::fromUtf8(startDescription->GetDescription().c_str())+"'";
  route.routeSteps.push_back(startAt);

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    RouteStep driveAlong;

    driveAlong.description="Drive along '"+QString::fromUtf8(nameDescription->GetDescription().c_str())+"'";
    route.routeSteps.push_back(driveAlong);
  }
}

void RoutingDialog::DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  RouteStep targetReached;

  targetReached.description="Target reached '"+QString::fromUtf8(targetDescription->GetDescription().c_str())+"'";
  route.routeSteps.push_back(targetReached);
}

void RoutingDialog::DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
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

    at.description="At crossing "+QString::fromUtf8(crossingWaysString.c_str());
    route.routeSteps.push_back(at);
  }

  RouteStep turn;

  if (directionDescription.Valid()) {
    turn.description=MoveToTurnCommand(directionDescription->GetCurve());
  }
  else {
    turn.description="Turn";
  }

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    turn.description+=" into '"+QString::fromUtf8(nameDescription->GetDescription().c_str())+"'";
  }

  route.routeSteps.push_back(turn);
}

void RoutingDialog::DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                                 const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep   at;

    at.description="At crossing ";
    at.description+=QString::fromUtf8(crossingWaysString.c_str());

    route.routeSteps.push_back(at);
  }

  RouteStep enter;

  enter.description="Enter roundabout";

  route.routeSteps.push_back(enter);
}

void RoutingDialog::DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                                 const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description="Leave roundabout (";
  leave.description+=roundaboutLeaveDescription->GetExitCount();
  leave.description+=". exit)";

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    leave.description+=" into street '";
    leave.description+=QString::fromUtf8(nameDescription->GetDescription().c_str());
    leave.description+="'";
  }

  route.routeSteps.push_back(leave);
}

void RoutingDialog::DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                               const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
{
  std::string crossingWaysString;

  if (crossingWaysDescription.Valid()) {
    crossingWaysString=CrossingWaysDescriptionToString(crossingWaysDescription);
  }

  if (!crossingWaysString.empty()) {
    RouteStep at;

    at.description="At crossing ";
    at.description+=QString::fromUtf8(crossingWaysString.c_str());
    route.routeSteps.push_back(at);
  }

  RouteStep enter;

  enter.description="Enter motorway";

  if (motorwayEnterDescription->GetToDescription().Valid() &&
      motorwayEnterDescription->GetToDescription()->HasName()) {
    enter.description+=" '";
    enter.description+=QString::fromUtf8(motorwayEnterDescription->GetToDescription()->GetDescription().c_str());
    enter.description+="'";
  }

  route.routeSteps.push_back(enter);
}

void RoutingDialog::DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
{
  RouteStep change;

  change.description="Change motorway";

  if (motorwayChangeDescription->GetFromDescription().Valid() &&
      motorwayChangeDescription->GetFromDescription()->HasName()) {
    change.description+=" from '";
    change.description+=QString::fromUtf8(motorwayChangeDescription->GetFromDescription()->GetDescription().c_str());
    change.description+="'";
  }

  if (motorwayChangeDescription->GetToDescription().Valid() &&
      motorwayChangeDescription->GetToDescription()->HasName()) {
    change.description+=" to '";
    change.description+=QString::fromUtf8(motorwayChangeDescription->GetToDescription()->GetDescription().c_str());
    change.description+="'";
  }

  route.routeSteps.push_back(change);
}

void RoutingDialog::DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                               const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                               const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  RouteStep leave;

  leave.description="Leave motorway";

  if (motorwayLeaveDescription->GetFromDescription().Valid() &&
      motorwayLeaveDescription->GetFromDescription()->HasName()) {
    leave.description+=" '";
    leave.description+=QString::fromUtf8(motorwayLeaveDescription->GetFromDescription()->GetDescription().c_str());
    leave.description+="'";
  }

  if (directionDescription.Valid() &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyLeft &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::straightOn &&
      directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyRight) {
    leave.description+=MoveToTurnCommand(directionDescription->GetCurve());
  }

  if (nameDescription.Valid() &&
      nameDescription->HasName()) {
    leave.description+=" into '";
    leave.description+=QString::fromUtf8(nameDescription->GetDescription().c_str());
    leave.description+="'";
  }

  route.routeSteps.push_back(leave);
}

void RoutingDialog::DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  RouteStep changed;

  changed.description="Way changes name";

  if (nameChangedDescription->GetOriginDesccription().Valid()) {
    changed.description+=" from '";
    changed.description+=QString::fromUtf8(nameChangedDescription->GetOriginDesccription()->GetDescription().c_str());
    changed.description+="'";
  }

  changed.description+="' to '";
  changed.description+=QString::fromUtf8(nameChangedDescription->GetTargetDesccription()->GetDescription().c_str());
  changed.description+="'";

  route.routeSteps.push_back(changed);
}

void RoutingDialog::Route()
{
  osmscout::RouteData routeData;
  osmscout::Way       routeWay;

  route.routeSteps.clear();
  routeModel->refresh();

  if (!dbThread->CalculateRoute(route.startObject,
                                route.startNodeIndex,
                                route.endObject,
                                route.endNodeIndex,
                                routeData)) {
    std::cerr << "There was an error while routing!" << std::endl;
    return;
  }

  dbThread->TransformRouteDataToRouteDescription(routeData,
                                                 route.routeDescription,
                                                 route.start.toUtf8().constData(),
                                                 route.end.toUtf8().constData());

  size_t                         roundaboutCrossingCounter=0;
  std::list<RouteStep>::iterator lastStep=route.routeSteps.end();

  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=route.routeDescription.Nodes().end();
  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=route.routeDescription.Nodes().begin();
       node!=route.routeDescription.Nodes().end();
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

    if (lastStep==route.routeSteps.end()) {
      current=route.routeSteps.begin();
    }
    else {
      current=lastStep;

      current++;
    }

    if (current!=route.routeSteps.end()) {
      current->distance=DistanceToString(node->GetDistance());
      current->time=TimeToString(node->GetTime());

      if (prevNode!=route.routeDescription.Nodes().end() &&
          node->GetDistance()-prevNode->GetDistance()!=0.0) {
        current->distanceDelta=DistanceToString(node->GetDistance()-prevNode->GetDistance());
      }
      if (prevNode!=route.routeDescription.Nodes().end() &&
          node->GetTime()-prevNode->GetTime()!=0.0) {
        current->timeDelta=TimeToString(node->GetTime()-prevNode->GetTime());
      }
    }

    while (current!=route.routeSteps.end()) {
      lastStep = current++;
    }

    prevNode=node;
  }

  if (dbThread->TransformRouteDataToWay(routeData,routeWay)) {
    dbThread->ClearRoute();
    dbThread->AddRoute(routeWay);
  }
  else {
    std::cerr << "Error while transforming route" << std::endl;
  }
}

#include "moc_RoutingDialog.cpp"
