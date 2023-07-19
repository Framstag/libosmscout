#ifndef OSMSCOUT_CLIENT_QT_QMLSETTINGS_H
#define OSMSCOUT_CLIENT_QT_QMLSETTINGS_H

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

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Provides interface to Settings object from QML. It should be registered
 * by qmlRegisterType before first use.
 *
 * ```
 * qmlRegisterType<QmlSettings>("net.sf.libosmscout.map", 1, 0, "Settings");
 * ```
 *
 * It may be imported and used in QML then:
 * ```
 * import net.sf.libosmscout.map 1.0
 *
 * Settings {
 *   id: settings
 * }
 * ```
 */
class OSMSCOUT_CLIENT_QT_API QmlSettings: public QObject{
Q_OBJECT
  Q_PROPERTY(double   physicalDPI READ GetPhysicalDPI CONSTANT)
  Q_PROPERTY(double   mapDPI    READ GetMapDPI  WRITE SetMapDPI   NOTIFY MapDPIChange)
  Q_PROPERTY(bool     onlineTiles READ GetOnlineTilesEnabled WRITE SetOnlineTilesEnabled NOTIFY OnlineTilesEnabledChanged)
  Q_PROPERTY(QString  onlineTileProviderId READ GetOnlineTileProviderId WRITE SetOnlineTileProviderId NOTIFY OnlineTileProviderIdChanged)
  Q_PROPERTY(bool     offlineMap READ GetOfflineMap WRITE SetOfflineMap NOTIFY OfflineMapChanged)
  Q_PROPERTY(QString  styleSheetFile READ GetStyleSheetFile WRITE SetStyleSheetFile NOTIFY StyleSheetFileChanged)
  Q_PROPERTY(bool     renderSea  READ GetRenderSea  WRITE SetRenderSea  NOTIFY RenderSeaChanged)
  Q_PROPERTY(QString  fontName    READ GetFontName            WRITE SetFontName     NOTIFY FontNameChanged)
  Q_PROPERTY(double   fontSize    READ GetFontSize            WRITE SetFontSize     NOTIFY FontSizeChanged)
  Q_PROPERTY(bool     showAltLanguage READ GetShowAltLanguage WRITE SetShowAltLanguage NOTIFY ShowAltLanguageChanged)
  /// metrics or imperial
  Q_PROPERTY(QString  units       READ GetUnits               WRITE SetUnits        NOTIFY UnitsChanged)

private:
  SettingsRef settings;

  // slots
  Slot<double> mapDPISlot{
    [this](const double &d){ this->MapDPIChange(d); }
  };

  Slot<bool> onlineTilesEnabledSlot{
    [this](const bool &b){this->OnlineTilesEnabledChanged(b);}
  };

  Slot<std::string> onlineTileProviderIdSlot{
    [this](const std::string &str){ this->OnlineTileProviderIdChanged(QString::fromStdString(str));}
  };

  Slot<bool> offlineMapSlot{
    [this](const bool &b){ this->OfflineMapChanged(b);}
  };

  Slot<std::string> styleSheetFileSlot{
    [this](const std::string &str){ this->StyleSheetFileChanged(QString::fromStdString(str));}
  };

  Slot<bool> renderSeaSlot{
    [this](const bool &b){ this->RenderSeaChanged(b);}
  };

  Slot<std::string> fontNameSlot{
    [this](const std::string &str){ this->FontNameChanged(QString::fromStdString(str));}
  };

  Slot<double> fontSizeSlot{
    [this](const double &d){ this->FontSizeChanged(d);}
  };

  Slot<bool> showAltLanguageSlot{
    [this](const bool &b){ this->ShowAltLanguageChanged(b);}
  };

  Slot<std::string> unitsSlot{
    [this](const std::string &str){ this->UnitsChanged(QString::fromStdString(str));}
  };

signals:
  void MapDPIChange(double dpi);
  void OnlineTilesEnabledChanged(bool enabled);
  void OnlineTileProviderIdChanged(const QString id);
  void OfflineMapChanged(bool);
  void StyleSheetFileChanged(const QString file);
  void RenderSeaChanged(bool);
  void FontNameChanged(const QString fontName);
  void FontSizeChanged(double fontSize);
  void ShowAltLanguageChanged(bool showAltLanguage);
  void UnitsChanged(const QString units);

public:
  QmlSettings();

  ~QmlSettings() override = default;

  double GetPhysicalDPI() const;

  void SetMapDPI(double dpi);
  double GetMapDPI() const;

  bool GetOnlineTilesEnabled() const;
  void SetOnlineTilesEnabled(bool b);

  const QString GetOnlineTileProviderId() const;
  void SetOnlineTileProviderId(QString id);

  Q_INVOKABLE QString onlineProviderCopyright();

  bool GetOfflineMap() const;
  void SetOfflineMap(bool);

  QString GetStyleSheetFile() const;
  void SetStyleSheetFile(const QString file);

  bool GetRenderSea() const;
  void SetRenderSea(bool);

  QString GetFontName() const;
  void SetFontName(const QString fontName);

  double GetFontSize() const;
  void SetFontSize(double fontSize);

  bool GetShowAltLanguage() const;
  void SetShowAltLanguage(bool showAltLanguage);

  QString GetUnits() const;
  void SetUnits(const QString units);
};

}

#endif //OSMSCOUT_CLIENT_QT_QMLSETTINGS_H
