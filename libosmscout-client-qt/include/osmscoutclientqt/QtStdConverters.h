#ifndef OSMSCOUT_CLIENT_QT_QTSTDCONVERTERS_H
#define OSMSCOUT_CLIENT_QT_QTSTDCONVERTERS_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2023 Lukas Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */


#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscout/util/Time.h>

#include <QDateTime>
#include <QStringList>
#include <QDir>

#include <filesystem>
#include <vector>

namespace osmscout {

extern OSMSCOUT_CLIENT_QT_API std::vector<std::string> QStringListToStringVector(const QStringList &list);

extern OSMSCOUT_CLIENT_QT_API QStringList StringVectorToQStringList(const std::vector<std::string> &v);

extern OSMSCOUT_CLIENT_QT_API QDateTime TimestampToQDateTime(const osmscout::Timestamp &ts);

extern OSMSCOUT_CLIENT_QT_API QList<QDir> PathVectorToQDirList(const std::vector<std::filesystem::path> &paths);

template <typename T>
QList<T> vectorToQList(const std::vector<T> &vec)
{
  QList<T> result;
  for (const auto &o: vec) {
    result << o;
  }
  return result;
}

}

#endif //OSMSCOUT_CLIENT_QT_QTSTDCONVERTERS_H
