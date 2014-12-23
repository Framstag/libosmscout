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

#include "SearchLocationModel.h"

#include <iostream>

#include "DBThread.h"

Location::Location(const QString& name,
                   QObject* parent)
    : QObject(parent),
      name(name)
{
    // no code
}

Location::Location(QObject* parent)
    : QObject(parent)
{
    // no code
}

Location::Location(const Location& other)
 : QObject(other.parent()),
   name(other.name),
   references(other.references)
{
    // no code
}

Location::~Location()
{
    // no code
}

void Location::addReference(const osmscout::ObjectFileRef reference)
{
    references.push_back(reference);
}

QString Location::getName() const
{
    return name;
}

const QList<osmscout::ObjectFileRef>& Location::getReferences() const
{
    return references;
}

LocationListModel::LocationListModel(QObject* parent)
: QAbstractListModel(parent)
{
    // no code
}

LocationListModel::~LocationListModel()
{
    for (QList<Location*>::iterator location=locations.begin();
         location!=locations.end();
         ++location) {
        delete *location;
    }

    locations.clear();
}

bool GetAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                             std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap,
                             std::string& path)
{
  if (!DBThread::GetInstance()->ResolveAdminRegionHierachie(adminRegion,
                                                            adminRegionMap)) {
    return false;
  }

  if (!adminRegion->aliasName.empty()) {
    if (!path.empty()) {
      path.append("/");
    }

    path.append(adminRegion->aliasName);
  }

  if (!path.empty()) {
    path.append("/");
  }

  path.append(adminRegion->name);

  osmscout::FileOffset parentRegionOffset=adminRegion->parentRegionOffset;

  while (parentRegionOffset!=0) {
    std::map<osmscout::FileOffset,osmscout::AdminRegionRef>::const_iterator entry=adminRegionMap.find(parentRegionOffset);

    if (entry==adminRegionMap.end()) {
      break;
    }

    osmscout::AdminRegionRef parentRegion=entry->second;

    if (!path.empty()) {
      path.append("/");
    }

    path.append(parentRegion->name);

    parentRegionOffset=parentRegion->parentRegionOffset;
  }

  return true;
}

QString GetAdminRegionLabel(std::map<osmscout::FileOffset,
                                osmscout::AdminRegionRef>& /*adminRegionMap*/,
                                const osmscout::AdminRegionRef& adminRegion)
{
  QString label;

  if (!adminRegion->aliasName.empty()) {
    label.append(QString::fromUtf8(adminRegion->aliasName.c_str()));
  }
  else {
    label.append(QString::fromUtf8(adminRegion->name.c_str()));
  }

  /*
  std::string path;

  if (!GetAdminRegionHierachie(adminRegion,
                               adminRegionMap,
                               path)) {
    return "";
  }

  if (!path.empty()) {
    label.append(" (");
    label.append(path);
    label.append(")");
  }*/

  return label;
}

void LocationListModel::setPattern(const QString& pattern)
{
  beginResetModel();

  for (QList<Location*>::iterator location=locations.begin();
       location!=locations.end();
       ++location) {
      delete *location;
  }

  locations.clear();

  osmscout::LocationSearchResult searchResult;

  std::string osmPattern=pattern.toUtf8().constData();

  std::cout << "Searching for '" << osmPattern << "'" << std::endl;

  DBThread::GetInstance()->SearchForLocations(osmPattern,
                                              50,
                                              searchResult);

  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;

  for (std::list<osmscout::LocationSearchResult::Entry>::const_iterator entry=searchResult.results.begin();
      entry!=searchResult.results.end();
      ++entry) {
      Location *location=NULL;

    if (entry->adminRegion.Valid() &&
        entry->location.Valid() &&
        entry->address.Valid()) {
      QString label=QString::fromUtf8(entry->location->name.c_str());

      label+=" ";
      label+=QString::fromUtf8(entry->address->name.c_str());

      label+=" ";
      label+=GetAdminRegionLabel(adminRegionMap,
                                 entry->adminRegion);

      std::cout << "- " << label.toLocal8Bit().data() << std::endl;

      location=new Location(label);
      location->addReference(entry->address->object);
    }
    else if (entry->adminRegion.Valid() &&
             entry->location.Valid()) {
      QString label=QString::fromUtf8(entry->location->name.c_str());

      label+=" ";
      label+=GetAdminRegionLabel(adminRegionMap,
                                 entry->adminRegion);

      std::cout << "- " << label.toLocal8Bit().data() << std::endl;

      location=new Location(label);

      for (std::vector<osmscout::ObjectFileRef>::const_iterator object=entry->location->objects.begin();
          object!=entry->location->objects.end();
          ++object) {
          location->addReference(*object);
      }
    }
    else if (entry->adminRegion.Valid() &&
             entry->poi.Valid()) {
      QString label=QString::fromUtf8(entry->poi->name.c_str());

      label+=" ";
      label+=GetAdminRegionLabel(adminRegionMap,
                                 entry->adminRegion);

      std::cout << "- " << label.toLocal8Bit().data() << std::endl;

      location=new Location(label);
      location->addReference(entry->poi->object);
    }
    else if (entry->adminRegion.Valid()) {
        QString label=GetAdminRegionLabel(adminRegionMap,
                                          entry->adminRegion);

        location=new Location(label);

        std::cout << "- " << label.toLocal8Bit().data() << std::endl;

        if (entry->adminRegion->aliasObject.Valid()) {
            location->addReference(entry->adminRegion->aliasObject);
        }
        else {
            location->addReference(entry->adminRegion->object);
        }
    }

    if (location!=NULL) {
        locations.append(location);
    }
  }

  endResetModel();
}

int LocationListModel::rowCount(const QModelIndex& ) const
{
    return locations.size();
}

QVariant LocationListModel::data(const QModelIndex &index, int role) const
{
    if(index.row() < 0 || index.row() >= locations.size()) {
        return QVariant();
    }

    Location* location=locations.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return location->getName();
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags LocationListModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> LocationListModel::roleNames() const
{
    QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

    roles[LabelRole]="label";

    return roles;
}

Location* LocationListModel::get(int row) const
{
    if(row < 0 || row >= locations.size()) {
        return NULL;
    }

    Location* location=locations.at(row);

    return new Location(*location);
}
