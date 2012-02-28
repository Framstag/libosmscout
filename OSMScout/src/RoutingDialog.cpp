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

#include "DBThread.h"
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

struct RouteSelection
{
  QString                    start;
  osmscout::Id               startWay;
  osmscout::Id               startNode;
  QString                    end;
  osmscout::Id               endWay;
  osmscout::Id               endNode;
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

RoutingDialog::RoutingDialog(QWidget* parentWindow)
 : QDialog(parentWindow,Qt::Dialog),
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

  SearchLocationDialog dialog(this);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location location;
    std::string        label;
    osmscout::WayRef   way;

    location=dialog.GetLocationResult();

    route.startWay=location.references.front().GetId();

    if (dbThread.GetWay(route.startWay,way)) {
      route.startNode=way->nodes[0].id;

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
      if (hasStart && hasEnd) {
        routeButton->setEnabled(true);
      }
    }
    else {
      route.start.clear();
      from->setText("");
    }
  }
}

void RoutingDialog::SelectTo()
{
  std::cout << "Select to..." << std::endl;

  SearchLocationDialog dialog(this);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location   location;
    std::string          label;
    osmscout::WayRef     way;

    location=dialog.GetLocationResult();

    route.endWay=location.references.front().GetId();

    if (dbThread.GetWay(route.endWay,way)) {
      route.endNode=way->nodes[0].id;

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
      if (hasStart && hasEnd) {
        routeButton->setEnabled(true);
      }
    }
    else {
      route.start.clear();
      from->setText("");
    }
  }
}

bool RoutingDialog::HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
{
  return node.HasDescription(osmscout::RouteDescription::NODE_START_DESC) ||
         node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC) ||
         node.HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
}

void RoutingDialog::PrepareRouteStep(const std::list<osmscout::RouteDescription::Node>::const_iterator& prevNode,
                                     const std::list<osmscout::RouteDescription::Node>::const_iterator& node,
                                     size_t lineCount,
                                     RouteStep& step)
{
  if (lineCount==0) {
    step.distance=DistanceToString(node->GetDistance());
    step.time=TimeToString(node->GetTime());

    if (prevNode!=route.routeDescription.Nodes().end() && node->GetDistance()-prevNode->GetDistance()!=0.0) {
      step.distanceDelta=DistanceToString(node->GetDistance()-prevNode->GetDistance());
    }

    if (prevNode!=route.routeDescription.Nodes().end() && node->GetTime()-prevNode->GetTime()!=0.0) {
      step.timeDelta=TimeToString(node->GetTime()-prevNode->GetTime());
    }
  }
}

void RoutingDialog::Route()
{
  std::cout << "Route" << std::endl;

  osmscout::RouteData        routeData;
  osmscout::WayRef           startWay;
  osmscout::WayRef           endWay;
  osmscout::Way              routeWay;

  if (!dbThread.GetWay(route.startWay,startWay)) {
    std::cerr << "Cannot load start way" << std::endl;
    return;
  }

  if (!dbThread.GetWay(route.endWay,endWay)) {
    std::cerr << "Cannot load end way" << std::endl;
    return;
  }

  if (!dbThread.CalculateRoute(startWay->GetId(),startWay->nodes.front().id,
                               endWay->GetId(),endWay->nodes.back().id,
                               routeData)) {
    std::cerr << "There was an error while routing!" << std::endl;
    return;
  }

  dbThread.TransformRouteDataToRouteDescription(routeData,
                                                route.routeDescription,
                                                route.start.toUtf8().constData(),
                                                route.end.toUtf8().constData());

  routeModel->refresh();

  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=route.routeDescription.Nodes().end();
  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=route.routeDescription.Nodes().begin();
       node!=route.routeDescription.Nodes().end();
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

      step.description="Start at \"" +QString::fromUtf8(startDescription->GetDescription().c_str()) + "\"";

      route.routeSteps.push_back(step);

      lineCount++;
    }
    if (node->HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
      osmscout::RouteDescription::DescriptionRef    description=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
      osmscout::RouteDescription::TargetDescription *targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(description.Get());
      RouteStep                                     step;

      PrepareRouteStep(prevNode,node,lineCount,step);
      step.description="Target reached \"" + QString::fromUtf8(targetDescription->GetDescription().c_str()) + "\"";

      route.routeSteps.push_back(step);

      lineCount++;
    }
    if (node->HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC)) {
      osmscout::RouteDescription::DescriptionRef  description=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
      osmscout::RouteDescription::NameDescription *nameDescription=dynamic_cast<osmscout::RouteDescription::NameDescription*>(description.Get());
      RouteStep                                   step;

      PrepareRouteStep(prevNode,node,lineCount,step);

      step.description="Way ";


      if (!nameDescription->GetName().empty()) {
        step.description+=QString::fromUtf8(nameDescription->GetName().c_str());
      }

      if (!nameDescription->GetName().empty() &&
          !nameDescription->GetRef().empty()) {
        step.description+=" (";
      }

      if (!nameDescription->GetRef().empty()) {
        step.description+=QString::fromUtf8(nameDescription->GetRef().c_str());
      }

      if (!nameDescription->GetName().empty() &&
          !nameDescription->GetRef().empty()) {
        step.description+=")";
      }

      route.routeSteps.push_back(step);

      lineCount++;
    }

    prevNode=node;
  }

  dbThread.TransformRouteDataToWay(routeData,routeWay);

  dbThread.ClearRoute();
  dbThread.AddRoute(routeWay);
}

#include "moc_RoutingDialog.cpp"
