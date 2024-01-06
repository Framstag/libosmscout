#ifndef OSMSCOUT_CLIENT_QT_MAPRENDERER_H
#define OSMSCOUT_CLIENT_QT_MAPRENDERER_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
 Copyright (C) 2017 Lukas Karas

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

#include <osmscoutmap/DataTileCache.h>

#include <osmscoutclient/DBThread.h>

#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/OverlayObject.h>
#include <osmscoutclientqt/DBLoadJob.h>

#include <QObject>
#include <QSettings>
#include <QMutex>
#include <QPainter>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API DBRenderJob : public QObject, public DBJob{
  Q_OBJECT
private:
  osmscout::MercatorProjection renderProjection;
  QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles;
  osmscout::MapParameter *drawParameter;
  QPainter *p;
  bool success;
  bool drawCanvasBackground;
  bool renderBasemap;
  bool renderDatabases;
  std::vector<OverlayObjectRef> overlayObjects;
  StyleConfigRef emptyStyleConfig;

public:
  DBRenderJob(osmscout::MercatorProjection renderProjection,
              QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles,
              osmscout::MapParameter *drawParameter,
              QPainter *p,
              std::vector<OverlayObjectRef> overlayObjects,
              StyleConfigRef emptyStyleConfig,
              bool drawCanvasBackground=true,
              bool renderBasemap=true,
              bool renderDatabases=true);

  ~DBRenderJob() override = default;

  void Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
           const std::list<DBInstanceRef> &allDatabases,
           ReadLock &&locker) override;

  inline bool IsSuccess() const{
    return success;
  };

private:
  bool addBasemapData(MapDataRef data) const;
  bool addOverlayObjectData(MapDataRef data, TypeConfigRef typeConfig) const;
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapRenderer : public QObject {
  Q_OBJECT

protected:
  QThread     *thread;
  SettingsRef settings;
  DBThreadRef dbThread;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0) /* For compatibility with QT 5.6 */
  QMutex          lock{QMutex::Recursive};
#else
  QRecursiveMutex lock;
#endif

  double      mapDpi;
  bool        renderSea;

  QString     fontName;
  double      fontSize;
  QString     iconDirectory;
  bool        showAltLanguage{false};
  QString     units;

  mutable QMutex                 overlayLock;
  std::map<int,OverlayObjectRef> overlayObjectMap; // <! map guarded by overlayLock, OverlayWay object is multithread

signals:
  void Redraw();
  void TriggerDrawMap();

  void mapDpiChangeSignal(double);
  void renderSeaSignal(bool);
  void fontNameSignal(QString);
  void fontSizeSignal(double);
  void showAltLanguageSignal(bool);
  void unitsSignal(QString);
  void stylesheetFilenameChanged();
  void databaseLoadFinished(const osmscout::GeoBox &geoBox);

private:
  // slots
  Slot<double> mapDpiChangeSlot{
    [this](const double &d){
      mapDpiChangeSignal(d);
    }
  };

  Slot<bool> renderSeaSlot{
    [this](const bool &b){
      renderSeaSignal(b);
    }
  };

  Slot<std::string> fontNameSlot{
    [this](const std::string &str){
      fontNameSignal(QString::fromStdString(str));
    }
  };

  Slot<double> fontSizeSlot{
    [this](const double &d){
      fontSizeSignal(d);
    }
  };

  Slot<bool> showAltLanguageSlot{
    [this](const bool &b){
      showAltLanguageSignal(b);
    }
  };

  Slot<std::string> unitsSlot{
    [this](const std::string &str){
      unitsSignal(QString::fromStdString(str));
    }
  };

  Slot<> stylesheetFilenameChangedSlot{ std::bind(&MapRenderer::stylesheetFilenameChanged, this) };
  Slot<GeoBox> databaseLoadFinishedSlot{ std::bind(&MapRenderer::databaseLoadFinished, this, std::placeholders::_1) };

public slots:
  virtual void Initialize() = 0;

  virtual void InvalidateVisualCache() = 0;
  virtual void onStylesheetFilenameChanged();
  virtual void onDatabaseLoaded(osmscout::GeoBox boundingBox) = 0;

  virtual void onMapDPIChange(double dpi);
  virtual void onRenderSeaChanged(bool);
  virtual void onFontNameChanged(const QString&);
  virtual void onFontSizeChanged(double);
  virtual void onShowAltLanguageChanged(bool);
  virtual void onUnitsChanged(const QString&);

protected:
  MapRenderer(QThread *thread,
              SettingsRef settings,
              DBThreadRef dbThread,
              QString iconDirectory);

  osmscout::GeoBox overlayObjectsBox() const;

  void getOverlayObjects(std::vector<OverlayObjectRef> &objs,
                         osmscout::GeoBox requestBox) const;

public:
  virtual ~MapRenderer();

  /**
   * Render map defined by request to painter
   * @param painter
   * @param request
   * @return true if rendered map is complete
   */
  virtual bool RenderMap(QPainter& painter,
                         const MapViewStruct& request) = 0;

  void addOverlayObject(int id, const OverlayObjectRef& obj);

  void removeOverlayObject(int id);
  void removeAllOverlayObjects();

  std::map<int,OverlayObjectRef> getOverlayObjects() const;
};

using MapRendererRef = std::shared_ptr<MapRenderer>;

}

#endif /* OSMSCOUT_CLIENT_QT_MAPRENDERER_H */
