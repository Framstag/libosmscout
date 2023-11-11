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

#include <osmscoutclientqt/QtStdConverters.h>

namespace osmscout {

std::vector<std::string> QStringListToStringVector(const QStringList &list)
{
  std::vector<std::string> result;
  for (const auto &str: list) {
    result.push_back(str.toStdString());
  }
  return result;
}

QStringList StringVectorToQStringList(const std::vector<std::string> &v)
{
  QStringList result;
  for (const auto &str: v) {
    result << QString::fromStdString(str);
  }
  return result;
}

QDateTime TimestampToQDateTime(const osmscout::Timestamp &ts)
{
  using namespace std::chrono;
  return QDateTime::fromMSecsSinceEpoch(duration_cast<milliseconds>(ts.time_since_epoch()).count());
}

QList<QDir> PathVectorToQDirList(const std::vector<std::filesystem::path> &paths)
{
  QList<QDir> result;
  for (const auto &path: paths) {
    result << QDir(QString::fromStdString(path.string()));
  }
  return result;
}

}
