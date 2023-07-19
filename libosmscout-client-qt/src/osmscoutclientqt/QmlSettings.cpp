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

#include <osmscoutclientqt/QmlSettings.h>
#include <osmscoutclientqt/OSMScoutQt.h>

namespace osmscout {

QmlSettings::QmlSettings()
{
  settings=OSMScoutQt::GetInstance().GetSettings();

  settings->mapDPIChange.Connect(mapDPISlot);
  settings->onlineTilesEnabledChanged.Connect(onlineTilesEnabledSlot);
  settings->onlineTileProviderIdChanged.Connect(onlineTileProviderIdSlot);
  settings->offlineMapChanged.Connect(offlineMapSlot);
  settings->styleSheetFileChanged.Connect(styleSheetFileSlot);
  settings->renderSeaChanged.Connect(renderSeaSlot);
  settings->fontNameChanged.Connect(fontNameSlot);
  settings->fontSizeChanged.Connect(fontSizeSlot);
  settings->showAltLanguageChanged.Connect(showAltLanguageSlot);
  settings->unitsChanged.Connect(unitsSlot);
}

double QmlSettings::GetPhysicalDPI() const
{
  return settings->GetPhysicalDPI();
}

void QmlSettings::SetMapDPI(double dpi)
{
  settings->SetMapDPI(dpi);
}

double QmlSettings::GetMapDPI() const
{
  return settings->GetMapDPI();
}

bool QmlSettings::GetOnlineTilesEnabled() const
{
  return settings->GetOnlineTilesEnabled();
}

void QmlSettings::SetOnlineTilesEnabled(bool b)
{
  settings->SetOnlineTilesEnabled(b);
}

const QString QmlSettings::GetOnlineTileProviderId() const
{
  return QString::fromStdString(settings->GetOnlineTileProviderId());
}

void QmlSettings::SetOnlineTileProviderId(QString id)
{
  settings->SetOnlineTileProviderId(id.toStdString());
}

QString QmlSettings::onlineProviderCopyright()
{
  OnlineTileProvider provider = settings->GetOnlineTileProvider();
  if (provider.isValid()){
    return QString::fromStdString(provider.getCopyright());
  }
  return "";
}

bool QmlSettings::GetOfflineMap() const
{
  return settings->GetOfflineMap();
}
void QmlSettings::SetOfflineMap(bool b)
{
  settings->SetOfflineMap(b);
}

QString QmlSettings::GetStyleSheetFile() const
{
  return QString::fromStdString(settings->GetStyleSheetFile());
}
void QmlSettings::SetStyleSheetFile(const QString file)
{
  settings->SetStyleSheetFile(file.toStdString());
}

bool QmlSettings::GetRenderSea() const
{
  return settings->GetRenderSea();
}
void QmlSettings::SetRenderSea(bool b)
{
  settings->SetRenderSea(b);
}
QString QmlSettings::GetFontName() const
{
  return QString::fromStdString(settings->GetFontName());
}
void QmlSettings::SetFontName(const QString fontName)
{
  settings->SetFontName(fontName.toStdString());
}
double QmlSettings::GetFontSize() const
{
  return settings->GetFontSize();
}
void QmlSettings::SetFontSize(double fontSize)
{
  settings->SetFontSize(fontSize);
}
bool QmlSettings::GetShowAltLanguage() const
{
  return settings->GetShowAltLanguage();
}
void QmlSettings::SetShowAltLanguage(bool showAltLanguage)
{
  settings->SetShowAltLanguage(showAltLanguage);
}
QString QmlSettings::GetUnits() const
{
  return QString::fromStdString(settings->GetUnits());
}
void QmlSettings::SetUnits(const QString units)
{
  settings->SetUnits(units.toStdString());
}

}
