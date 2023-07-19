/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2023  Lukas Karas

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

#include <osmscoutclientqt/QtSettingsStorage.h>

namespace osmscout {


QtSettingsStorage::QtSettingsStorage(QSettings *providedStorage):
  storage(providedStorage)
{
  if (storage==nullptr){
    storage=new QSettings(this);
  } else {
    providedStorage->setParent(this);
  }
}

void QtSettingsStorage::SetValue(const std::string &key, double d)
{
  storage->setValue(QString::fromStdString(key), d);
}

void QtSettingsStorage::SetValue(const std::string &key, uint32_t i)
{
  storage->setValue(QString::fromStdString(key), i);
}

void QtSettingsStorage::SetValue(const std::string &key, const std::string &str)
{
  storage->setValue(QString::fromStdString(key), QString::fromStdString(str));
}

void QtSettingsStorage::SetValue(const std::string &key, bool b)
{
  storage->setValue(QString::fromStdString(key), b);
}

void QtSettingsStorage::SetValue(const std::string &key, std::vector<char> bytes)
{
  storage->setValue(QString::fromStdString(key), QByteArray(bytes.data(), bytes.size()));
}

double QtSettingsStorage::GetDouble(const std::string &key, double defaultValue)
{
  bool ok;
  double d = storage->value(QString::fromStdString(key), defaultValue).toDouble(&ok);
  if (!ok) {
    return defaultValue;
  }
  return d;
}

uint32_t QtSettingsStorage::GetUInt(const std::string &key, uint32_t defaultValue)
{
  bool ok;
  uint32_t i = storage->value(QString::fromStdString(key), defaultValue).toUInt(&ok);
  if (!ok) {
    return defaultValue;
  }
  return i;
}

std::string QtSettingsStorage::GetString(const std::string &key, const std::string &defaultValue)
{
  return storage->value(QString::fromStdString(key), QString::fromStdString(defaultValue)).toString().toStdString();
}

bool QtSettingsStorage::GetBool(const std::string &key, bool defaultValue)
{
  return storage->value(QString::fromStdString(key), defaultValue).toBool();
}

std::vector<char> QtSettingsStorage::GetBytes(const std::string &key)
{
  QByteArray arr = storage->value(QString::fromStdString(key)).toByteArray();
  return std::vector<char>(arr.data(), arr.data() + arr.size());
}

std::vector<std::string> QtSettingsStorage::Keys(const std::string &prefix)
{
  std::vector<std::string> result;
  storage->beginGroup(QString::fromStdString(prefix));
  for (const QString& key:storage->allKeys()){
    result.push_back(prefix + key.toStdString());
  }
  storage->endGroup();
  return result;
}

}
