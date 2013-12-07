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

class LocationItem : public QStandardItem
{
public:
  osmscout::LocationSearchResult::Entry entry;
  osmscout::ObjectFileRef               object;

  LocationItem(osmscout::LocationSearchResult::Entry entry)
  : entry(entry)
  {
    std::string label;

    if (entry.adminRegion.Valid() &&
        entry.location.Valid() &&
        entry.address.Valid()) {
      label=entry.location->name+" "+entry.address->name+" "+entry.adminRegion->name;
      object=entry.address->object;
      /*
      std::cout << " " << GetAdminRegionLabel(database,
                                              adminRegionMap,
                                              *entry);*/
    }
    else if (entry.adminRegion.Valid() &&
             entry.location.Valid()) {
      label=entry.location->name+" "+entry.adminRegion->name;
      object=entry.location->objects.front();
    }
    else if (entry.adminRegion.Valid() &&
             entry.poi.Valid()) {
      label=entry.poi->name+" "+entry.adminRegion->name;
      object=entry.poi->object;
    }
    else if (entry.adminRegion.Valid()) {
      if (entry.adminRegion->aliasObject.Valid()) {
        label=entry.adminRegion->aliasName;
        object=entry.adminRegion->aliasObject;
      }
      else {
        label=entry.adminRegion->name;
        object=entry.adminRegion->object;
      }
    }

    setText(QString::fromUtf8(label.c_str()));
  }
};

SearchLocationDialog::SearchLocationDialog(QWidget* parentWindow,
                                           DBThread* dbThread)
 : QDialog(parentWindow,Qt::Dialog),
   dbThread(dbThread),
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
  results->setMinimumWidth(QApplication::fontMetrics().width("mlxi")/4*70);

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

    resultLocation=item->object;
    resultLocationName=item->text();
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
  osmscout::LocationSearch       search;
  osmscout::LocationSearchResult result;

  search.InitializeSearchEntries(locationName->text().toUtf8().constData());
  search.limit=50;

  dbThread->SearchForLocations(search,
                               result);

  if (result.limitReached) {
    // TODO: Set info text or simply show what we have found?
    okButton->setEnabled(false);
    return;
  }
  else if (result.results.empty()) {
    okButton->setEnabled(false);
    return;
  }

  locations->clear();

  for (std::list<osmscout::LocationSearchResult::Entry>::const_iterator entry=result.results.begin();
      entry!=result.results.end();
      ++entry) {
    locations->appendRow(new LocationItem(*entry));
  }
}

osmscout::ObjectFileRef SearchLocationDialog::GetLocationResult() const
{
  return resultLocation;
}

QString SearchLocationDialog::GetLocationResultName() const
{
  return resultLocationName;
}

#include "moc_SearchLocationDialog.cpp"
