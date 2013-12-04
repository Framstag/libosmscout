#ifndef SEARCHLOCATION_DIALOG_H
#define SEARCHLOCATION_DIALOG_H

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

#include <QDialog>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTimer>

#include <osmscout/Location.h>

#include <DBThread.h>

class SearchLocationDialog : public QDialog
{
  Q_OBJECT

private:
  DBThread                *dbThread;

  QLineEdit               *locationName;
  QListView               *results;
  QStandardItemModel      *locations;
  QPushButton             *okButton;
  QTimer                  requestResultTimer;

  osmscout::ObjectFileRef resultLocation;
  QString                 resultLocationName;

public slots:
  void OnLocationNameChange(const QString& text);
  void OnSelectionChanged(const QItemSelection& selected,
                          const QItemSelection& deselected);
  void OnDoubleClick(const QModelIndex& index);
  void Search();

public:
  SearchLocationDialog(QWidget* parentWindow,
                       DBThread* dbThread);
  ~SearchLocationDialog();

  osmscout::ObjectFileRef GetLocationResult() const;
  QString GetLocationResultName() const;
};

#endif
