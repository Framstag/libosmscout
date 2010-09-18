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

RoutingDialog::RoutingDialog(QWidget* parentWindow)
 : QDialog(parentWindow,Qt::Dialog),
   from(new QLineEdit()),
   to(new QLineEdit()),
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

    fromLocation=dialog.GetLocationResult();

    if (fromLocation.path.empty()) {
      label=fromLocation.name;
    }
    else {
      label=fromLocation.name+" ("+osmscout::StringListToString(fromLocation.path)+")";
    }

    from->setText(QString::fromUtf8(label.c_str()));
    // Make sure, start of text is visible
    from->setCursorPosition(0);
  }

  if (fromLocation.IsValid() &&
      toLocation.IsValid() &&
      fromLocation.references.front().GetType()==osmscout::refWay &&
      toLocation.references.front().GetType()==osmscout::refWay) {
    routeButton->setEnabled(true);
  }
}

void RoutingDialog::SelectTo()
{
  std::cout << "Select to..." << std::endl;

  SearchLocationDialog dialog(this);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location location;
    std::string        label;

    toLocation=dialog.GetLocationResult();

    if (toLocation.path.empty()) {
      label=toLocation.name;
    }
    else {
      label=toLocation.name+" ("+osmscout::StringListToString(toLocation.path)+")";
    }

    to->setText(QString::fromUtf8(label.c_str()));
    // Make sure, start of text is visible
    to->setCursorPosition(0);
  }

  if (fromLocation.IsValid() &&
      toLocation.IsValid() &&
      fromLocation.references.front().GetType()==osmscout::refWay &&
      toLocation.references.front().GetType()==osmscout::refWay) {
    routeButton->setEnabled(true);
  }
}

void RoutingDialog::Route()
{
  std::cout << "Route" << std::endl;

  osmscout::RouteData        routeData;
  osmscout::RouteDescription routeDescription;
  osmscout::Way              startWay;
  osmscout::Way              endWay;
  osmscout::Way              routeWay;

  if (!dbThread.GetWay(fromLocation.references.front().id,startWay)) {
    std::cerr << "Cannot load start way" << std::endl;
    return;
  }

  if (!dbThread.GetWay(toLocation.references.front().id,endWay)) {
    std::cerr << "Cannot load end way" << std::endl;
    return;
  }

  if (!dbThread.CalculateRoute(startWay.id,startWay.nodes.front().id,
                               endWay.id,endWay.nodes.back().id,
                               routeData)) {
    std::cerr << "There was an error while routing!" << std::endl;
    return;
  }

  //databaseTask->DumpStatistics();

  dbThread.TransformRouteDataToRouteDescription(routeData,routeDescription);

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
