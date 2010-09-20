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

#include <iostream>
#include <iomanip>

static std::string DistanceToString(double distance)
{
  std::ostringstream stream;

  stream.setf(std::ios::fixed);
  stream.precision(1);
  stream << distance << "km";

  return stream.str();
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
};

RouteSelection route;

RouteModel::RouteModel(QObject *parent)
 :QAbstractTableModel(parent)
{
  // no code
}

int RouteModel::rowCount(const QModelIndex &parent) const
{
  return route.routeDescription.Steps().size();
}

int RouteModel::columnCount(const QModelIndex &parent) const
{
  return 3;
}

QVariant RouteModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()) {
    return QVariant();
  }

  if (index.row()<0 || index.row()>=(int)route.routeDescription.Steps().size()) {
    return QVariant();
  }

  if (role==Qt::DisplayRole) {
    std::list<osmscout::RouteDescription::RouteStep>::const_iterator step=route.routeDescription.Steps().begin();

    // Not fast, but hopefully fast enough for small lists
    std::advance(step,index.row());

    if (index.column()==0) {
      return DistanceToString(step->GetAt()).c_str();
    }
    else if (index.column()==1) {
      return DistanceToString(step->GetAfter()).c_str();
    }
    else if (index.column()==2) {
      QString action;
      switch (step->GetAction()) {
      case osmscout::RouteDescription::start:
        action="Start at ";
        break;
      case osmscout::RouteDescription::drive:
        action="Drive along ";
        break;
      case osmscout::RouteDescription::switchRoad:
        action="Turn into ";
        break;
      case osmscout::RouteDescription::reachTarget:
        action="Arriving at ";
        break;
      case osmscout::RouteDescription::pass:
        action="Passing along ";
        break;
      }

      if (!step->GetName().empty()) {
        action+=QString::fromUtf8(step->GetName().c_str());

        if (!step->GetRefName().empty()) {
          action+=" ("+QString::fromUtf8(step->GetRefName().c_str())+")";
        }
      }
      else {
        action+=QString::fromUtf8(step->GetRefName().c_str());
      }

      return action;
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
    osmscout::Way      way;

    location=dialog.GetLocationResult();

    route.startWay=location.references.front().GetId();

    if (dbThread.GetWay(route.startWay,way)) {
      route.startNode=way.nodes[0].id;

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
    osmscout::Way        way;

    location=dialog.GetLocationResult();

    route.endWay=location.references.front().GetId();

    if (dbThread.GetWay(route.endWay,way)) {
      route.endNode=way.nodes[0].id;

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

void RoutingDialog::Route()
{
  std::cout << "Route" << std::endl;

  osmscout::RouteData        routeData;
  osmscout::Way              startWay;
  osmscout::Way              endWay;
  osmscout::Way              routeWay;

  if (!dbThread.GetWay(route.startWay,startWay)) {
    std::cerr << "Cannot load start way" << std::endl;
    return;
  }

  if (!dbThread.GetWay(route.endWay,endWay)) {
    std::cerr << "Cannot load end way" << std::endl;
    return;
  }

  if (!dbThread.CalculateRoute(startWay.id,startWay.nodes.front().id,
                               endWay.id,endWay.nodes.back().id,
                               routeData)) {
    std::cerr << "There was an error while routing!" << std::endl;
    return;
  }

  dbThread.TransformRouteDataToRouteDescription(routeData,route.routeDescription);

  routeModel->refresh();

  for (std::list<osmscout::RouteDescription::RouteStep>::const_iterator step=route.routeDescription.Steps().begin();
       step!=route.routeDescription.Steps().end();
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

  dbThread.TransformRouteDataToWay(routeData,routeWay);

  dbThread.ClearRoute();
  dbThread.AddRoute(routeWay);
}

#include "moc_RoutingDialog.cpp"
