#ifndef OSMSCOUT_CLIENT_QT_QTSETTINGSSTORAGE_H
#define OSMSCOUT_CLIENT_QT_QTSETTINGSSTORAGE_H

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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclient/Settings.h>

#include <QObject>
#include <QSettings>


namespace osmscout {

  class OSMSCOUT_CLIENT_QT_API QtSettingsStorage: public QObject, public SettingsStorage
  {
  private:
    QSettings *storage;
  public:
    /**
     * @param providedStorage custom provider when not null. Storage takes ownership.
     *    When it is null, default Qt settings is used.
     */
    explicit QtSettingsStorage(QSettings *providedStorage=nullptr);
    virtual ~QtSettingsStorage() = default;

    void SetValue(const std::string &key, double d) override;
    void SetValue(const std::string &key, uint32_t i) override;
    void SetValue(const std::string &key, const std::string &str) override;
    void SetValue(const std::string &key, bool b) override;
    void SetValue(const std::string &key, std::vector<char> bytes) override;
    double GetDouble(const std::string &key, double defaultValue = 0) override;
    uint32_t GetUInt(const std::string &key, uint32_t defaultValue = 0) override;
    std::string GetString(const std::string &key, const std::string &defaultValue = "") override;
    bool GetBool(const std::string &key, bool defaultValue = 0) override;
    std::vector<char> GetBytes(const std::string &key) override;

    std::vector<std::string> Keys(const std::string &prefix) override;
  };

}
#endif //OSMSCOUT_CLIENT_QT_QTSETTINGSSTORAGE_H
