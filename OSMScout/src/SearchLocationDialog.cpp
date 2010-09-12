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

#include "SearchLocationDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

#include "DBThread.h"

class LocationItem : public QStandardItem
{
public:
  osmscout::Location location;

  LocationItem(const osmscout::Location& location)
  : location(location)
  {
    std::string label;

    if (location.path.empty()) {
      label=location.name;
    }
    else {
      label=location.name+" ("+osmscout::StringListToString(location.path)+")";
    }

    setText(QString::fromUtf8(label.c_str()));
  }
};

SearchLocationDialog::SearchLocationDialog(QWidget* parentWindow)
 : QDialog(parentWindow,Qt::Dialog),
   locationName(new QLineEdit()),
   results(new QListView()),
   locations(new QStandardItemModel())
{
  requestResultTimer.setSingleShot(true);

  results->setModel(locations);
  results->setUniformItemSizes(true);
  results->setEditTriggers(QAbstractItemView::NoEditTriggers);
  results->setViewMode(QListView::ListMode);
  results->setSelectionMode(QAbstractItemView::SingleSelection);
  results->setSelectionBehavior(QAbstractItemView::SelectRows);

  QVBoxLayout *mainLayout=new QVBoxLayout();
  QFormLayout *formLayout=new QFormLayout();

  formLayout->addRow("Location:",locationName);
  formLayout->addRow("Hits:",results);

  mainLayout->addLayout(formLayout);

  QDialogButtonBox *buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok |
                                                   QDialogButtonBox::Cancel);

  okButton=buttonBox->button(QDialogButtonBox::Ok);

  okButton->setEnabled(false);

  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  connect(locationName,SIGNAL(textChanged(QString)),this, SLOT(OnLocationNameChange(QString)));
  connect(&requestResultTimer,SIGNAL(timeout()),this, SLOT(Search()));
  connect(buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
  connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
  connect(results->selectionModel(),
          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this,
          SLOT(OnSelectionChanged(QItemSelection,QItemSelection)));
  connect(results,
          SIGNAL(doubleClicked(QModelIndex)),
          this,
          SLOT(OnDoubleClick(QModelIndex)));
}

SearchLocationDialog::~SearchLocationDialog()
{
  // no code
}

void SearchLocationDialog::OnLocationNameChange(const QString& text)
{
  requestResultTimer.stop();
  requestResultTimer.start(2000);
}

void SearchLocationDialog::OnSelectionChanged(const QItemSelection& selected,
                                              const QItemSelection& deselected)
{
  if (results->selectionModel()->hasSelection() &&
      results->selectionModel()->selection().indexes().size()==1) {
    okButton->setEnabled(true);

    QModelIndex  index=results->selectionModel()->selection().indexes().front();
    LocationItem *item=dynamic_cast<LocationItem*>(locations->itemFromIndex(index));

    locationResult=item->location;
  }
  else {
    okButton->setEnabled(false);
  }

}

void SearchLocationDialog::OnDoubleClick(const QModelIndex& index)
{
  if (okButton->isEnabled()) {
    accept();
  }
}

void SearchLocationDialog::Search()
{
  bool    limitReached=true;
  int     dividerPos;
  QString location(locationName->text());
  QString city;
  QString street;

  dividerPos=location.indexOf(',');
  if (dividerPos>=0) {
    city=location.mid(dividerPos+1);
    street=location.mid(0,dividerPos);
  }
  else {
    city=location;
  }

  street=street.trimmed();
  city=city.trimmed();

  if (!street.isEmpty()) {
    std::cout << "Searching for street '" << street.toUtf8().data() << "' in city '" << city.toUtf8().data() << "'..." << std::endl;
  }
  else {
    std::cout << "Searching for city '" << city.toUtf8().data() << "'..." << std::endl;
  }

  std::list<osmscout::AdminRegion> regions;

  locations->clear();

  if (!locationName->text().isEmpty()) {
    dbThread.GetMatchingAdminRegions(city,regions,50,limitReached);

    std::cout << "Result of search for region " << city.toUtf8().data() << ": " << regions.size() << std::endl;

    if (limitReached) {
      //locationsModel->SetEmptyText(L"- too many hits -");
      okButton->setEnabled(false);
      return;
    }
    else if (regions.size()==0) {
      //locationsModel->SetEmptyText(L"- no matches -");
      okButton->setEnabled(false);
      return;
    }
    else if (street.isEmpty()) {
      //locationsModel->SetEmptyText(L"");
    }
  }
  else {
    //locationsModel->SetEmptyText(L"- no search criteria -");
    okButton->setEnabled(false);
    return;
  }

  if (street.isEmpty()) {
    for (std::list<osmscout::AdminRegion>::const_iterator region=regions.begin();
         region!=regions.end();
         ++region) {
      osmscout::Location location;

      location.name=region->name;
      location.references.push_back(region->reference);
      location.path=region->path;

      locations->appendRow(new LocationItem(location));
    }

    return;
  }


  for (std::list<osmscout::AdminRegion>::const_iterator region=regions.begin();
       region!=regions.end();
       ++region) {
    std::list<osmscout::Location> locs;

    dbThread.GetMatchingLocations(*region,
                                  street,
                                  locs,
                                  50,
                                  limitReached);

    std::cout << "Result of search for street " << street.toUtf8().data() << " in region " << region->name << ": " << locs.size() << std::endl;

    if (limitReached) {
      std::cout << "Limit reached." << std::endl;
      locations->clear();
      //locationsModel->SetEmptyText(L"- too many hits -");
      okButton->setEnabled(false);

      return;
    }

    for (std::list<osmscout::Location>::const_iterator l=locs.begin();
         l!=locs.end();
         ++l) {
      bool found=false;

      for (int l2=0; l2<locations->rowCount(); l2++) {
        LocationItem* item=dynamic_cast<LocationItem*>(locations->item(l2));

        if (item->location.references.front()==l->references.front()) {
          found=true;
          break;
        }
      }

      if (found) {
        continue;
      }

      locations->appendRow(new LocationItem(*l));

      if (locations->rowCount()>50) {
        std::cout << "Limit reached." << std::endl;
        locations->clear();
        //locationsModel->SetEmptyText(L"- too many hits -");
        okButton->setEnabled(false);

        return;
      }
    }

  }

  if (locations->rowCount()==0) {
    std::cout << "No matches." << std::endl;
    //locationsModel->SetEmptyText(L"- no matches -");
    okButton->setEnabled(false);

    return;
  }
  else {
    //locationsModel->SetEmptyText(L"");
  }
}

osmscout::Location SearchLocationDialog::GetLocationResult() const
{
  return locationResult;
}
#include "moc_SearchLocationDialog.cpp"
