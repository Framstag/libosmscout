/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2022 Lukas Karas

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

#include <osmscoutclientqt/IconLookup.h>
#include <QSvgRenderer>
#include "osmscoutmapqt/SymbolRendererQt.h"

namespace osmscout {

IconLookup::IconLookup(QThread *thread, DBThreadRef dbThread, QString iconDirectory):
  QObject(),
  thread(thread),
  dbThread(dbThread),
  loadJob(nullptr)
{
  std::list<std::string> paths;
  paths.push_back(iconDirectory.toStdString());

  drawParameter.SetIconMode(osmscout::MapParameter::IconMode::Scalable);
  drawParameter.SetPatternMode(osmscout::MapParameter::PatternMode::Scalable);
  drawParameter.SetIconPaths(paths);
  drawParameter.SetPatternPaths(paths);

  connect(this, &IconLookup::iconRequested,
          this, &IconLookup::onIconRequest,
          Qt::QueuedConnection);
}

IconLookup::~IconLookup()
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Destroy" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread != nullptr) {
    thread->quit();
  }
}

void IconLookup::lookupIcons(const QString &databasePath,
                             osmscout::MapData &data,
                             const TypeConfigRef &typeConfig,
                             const StyleConfigRef &styleConfig)
{
  for (auto const &o:overlayObjects){
    if (o->getObjectType()==osmscout::RefType::refWay){
      OverlayWay *ow=dynamic_cast<OverlayWay*>(o.get());
      if (ow != nullptr) {
        osmscout::WayRef w = std::make_shared<osmscout::Way>();
        if (ow->toWay(w, *typeConfig)) {
          data.poiWays.push_back(w);
        }
      }
    } else if (o->getObjectType()==osmscout::RefType::refArea){
      OverlayArea *oa=dynamic_cast<OverlayArea*>(o.get());
      if (oa != nullptr) {
        osmscout::AreaRef a = std::make_shared<osmscout::Area>();
        if (oa->toArea(a, *typeConfig)) {
          data.poiAreas.push_back(a);
        }
      }
    } else if (o->getObjectType()==osmscout::RefType::refNode){
      OverlayNode *oo=dynamic_cast<OverlayNode*>(o.get());
      if (oo != nullptr) {
        osmscout::NodeRef n = std::make_shared<osmscout::Node>();
        if (oo->toNode(n, *typeConfig)) {
          data.poiNodes.push_back(n);
        }
      }
    }
  }
  overlayObjects.clear();

  double iconSize = projection.ConvertWidthToPixel(drawParameter.GetIconSize());

  auto CheckIcon=[&](const IconStyleRef &iconStyle,
                     const GeoCoord &coord,
                     const ObjectFileRef &objectRef,
                     const FeatureValueBuffer& featureBuffer) {
    if (iconStyle && iconStyle->IsVisible() && !iconStyle->IsOverlay()) {
      double x, y;
      projection.GeoToPixel(coord, x, y);
      QRectF iconRect;

      if (!iconStyle->GetIconName().empty()) {
        iconRect=QRectF(x - iconSize/2, y-iconSize/2, iconSize, iconSize);
      } else {
        auto symbol=iconStyle->GetSymbol();
        assert(symbol);
        double w=symbol->GetWidth(projection);
        double h=symbol->GetHeight(projection);
        iconRect=QRectF(x - w/2, y-h/2, w, h);
      }
      if (iconRect.contains(lookupCoord)) {

        QString name;
        QString phone;
        QString website;
        if (const osmscout::NameFeatureValue *nameValue=featureBuffer.findValue<osmscout::NameFeatureValue>();
            nameValue!=nullptr){
          name=QString::fromStdString(nameValue->GetLabel(Locale(), 0));
        }
        if (const osmscout::PhoneFeatureValue *phoneValue=featureBuffer.findValue<osmscout::PhoneFeatureValue>();
            phoneValue!=nullptr){
          phone=QString::fromStdString(phoneValue->GetPhone());
        }
        if (const osmscout::WebsiteFeatureValue *websiteValue=featureBuffer.findValue<osmscout::WebsiteFeatureValue>();
            websiteValue!=nullptr){
          website=QString::fromStdString(websiteValue->GetWebsite());
        }

        findIcons.push_back(MapIcon{QPoint(x,y), iconRect, coord, iconStyle,
                                    databasePath, objectRef,
                                    name, phone, website, QImage()});
      }
    }
  };

  auto VisitArea=[&](const AreaRef a) {
    a->VisitRings([&](size_t, const Area::Ring&r, const TypeInfoRef& type) -> bool {
      if (!type->GetIgnore()) {
        auto iconStyle = styleConfig->GetAreaIconStyle(type, r.GetFeatureValueBuffer(), projection);
        auto coord = r.center.value_or(r.GetBoundingBox().GetCenter());
        CheckIcon(iconStyle, coord, a->GetObjectFileRef(), a->GetFeatureValueBuffer());
      }
      return true;
    });
  };

  for (auto const &n:data.nodes) {
    auto iconStyle = styleConfig->GetNodeIconStyle(n->GetFeatureValueBuffer(), projection);
    CheckIcon(iconStyle, n->GetCoords(), n->GetObjectFileRef(), n->GetFeatureValueBuffer());
  }
  for (auto const &n:data.poiNodes) {
    auto iconStyle = styleConfig->GetNodeIconStyle(n->GetFeatureValueBuffer(), projection);
    CheckIcon(iconStyle, n->GetCoords(), n->GetObjectFileRef(), n->GetFeatureValueBuffer());
  }
  for (auto const &a:data.areas) {
    VisitArea(a);
  }
  for (auto const &a:data.poiAreas) {
    VisitArea(a);
  }
}

void IconLookup::onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles)
{
  osmscout::MapData data;
  loadJob->AddTileDataToMapData(dbPath,tiles,data);
  dbThread->RunSynchronousJob([&](const std::list<DBInstanceRef> &databases){
    for (auto &db: databases) {
      if (db->path==dbPath){
        TypeConfigRef typeConfig=db->GetDatabase()->GetTypeConfig();
        StyleConfigRef styleConfig=db->GetStyleConfig();
        this->lookupIcons(db->path, data, typeConfig, styleConfig);
      }
    }
  });
}


void IconLookup::onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> /*tiles*/)
{
  std::stable_sort(findIcons.begin(), findIcons.end(), [](const MapIcon &a, const MapIcon &b){
    if (a.iconStyle->GetPriority() == b.iconStyle->GetPriority()) {
      return a.screenCoord.x() < b.screenCoord.x();
    }
    return a.iconStyle->GetPriority() < b.iconStyle->GetPriority();
  });
  if (!findIcons.empty()){

    auto &findIcon=findIcons[0];
    if (!findIcon.iconStyle->GetIconName().empty()) {
      for (const auto &path: drawParameter.GetIconPaths()) {
        std::string filename = AppendFileToDir(path, findIcon.iconStyle->GetIconName() + ".svg");

        // Load SVG
        QSvgRenderer renderer(QString::fromStdString(filename));
        if (renderer.isValid()) {
          findIcon.image = QImage(findIcon.dimensions.width()*iconImageUpscale,
                                  findIcon.dimensions.height()*iconImageUpscale,
                                  QImage::Format_ARGB32);
          findIcon.image.fill(Qt::transparent);

          QPainter painter(&findIcon.image);
          renderer.render(&painter);
          painter.end();
          if (!findIcon.image.isNull()) {
            break;
          }
        }
      }
    } else {
      findIcon.image = QImage(findIcon.dimensions.width()*iconImageUpscale,
                              findIcon.dimensions.height()*iconImageUpscale,
                              QImage::Format_ARGB32);
      findIcon.image.fill(Qt::transparent);

      QPainter painter(&findIcon.image);
      SymbolRendererQt renderer(&painter);
      SymbolRef symbol=findIcon.iconStyle->GetSymbol();

      double minX, minY, maxX, maxY;
      symbol->GetBoundingBox(projection,minX,minY,maxX,maxY);
      renderer.Render(*symbol,
                      Vertex2D(minX*-1*iconImageUpscale,minY*-1*iconImageUpscale),
                      projection.GetMeterInPixel()*iconImageUpscale,
                      projection.ConvertWidthToPixel(iconImageUpscale));
      painter.end();
    }

    emit iconFound(lookupCoord, findIcon);
  } else {
    emit iconNotFound(lookupCoord);
  }

  overlayObjects.clear();
  findIcons.clear();
}

void IconLookup::onIconRequest(const MapViewStruct &view,
                               const QPoint &coord,
                               const std::map<int,OverlayObjectRef> &overlayObjectMap)
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Request from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  if (loadJob!=nullptr){
    delete loadJob;
  }

  // setup projection for data lookup
  projection.Set(view.coord, view.angle.AsRadians(), view.magnification, view.dpi, view.width, view.height);
  projection.SetLinearInterpolationUsage(view.magnification.GetLevel() >= 10);

  overlayObjects.clear();
  for (const auto &e: overlayObjectMap) {
    overlayObjects.push_back(e.second);
  }

  unsigned long maximumAreaLevel=4;
  if (view.magnification.GetLevel() >= 15) {
    maximumAreaLevel=6;
  }

  lookupCoord=coord;
  loadJob=new DBLoadJob(projection, maximumAreaLevel,/* lowZoomOptimization */ true);

  connect(loadJob, &DBLoadJob::databaseLoaded,
          this, &IconLookup::onDatabaseLoaded);
  connect(loadJob, &DBLoadJob::finished,
          this, &IconLookup::onLoadJobFinished);

  dbThread->RunJob(loadJob);
}


void IconLookup::RequestIcon(const MapViewStruct &view,
                             const QPoint &coord,
                             const std::map<int,OverlayObjectRef> &overlayObjects)
{
  emit iconRequested(view, coord, overlayObjects);
}
}
