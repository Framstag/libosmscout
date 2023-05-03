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

#include <cmath>

#include <QSvgRenderer>

#include <osmscout/feature/OpeningHoursFeature.h>
#include <osmscout/feature/OperatorFeature.h>
#include <osmscout/feature/PhoneFeature.h>
#include <osmscout/feature/WebsiteFeature.h>

#include <osmscoutmapqt/SymbolRendererQt.h>



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
  double iconSize = projection.ConvertWidthToPixel(drawParameter.GetIconSize());
  double tapSizePixels = tapSize*dbThread->GetPhysicalDpi()/25.4;
  QRectF tapRectangle(lookupCoord.x()-tapSizePixels/2, lookupCoord.y()-tapSizePixels/2, tapSizePixels, tapSizePixels);

  auto CheckIcon=[&](const IconStyleRef &iconStyle,
                     const GeoCoord &coord,
                     const ObjectFileRef &objectRef,
                     int poiId,
                     const TypeInfoRef& type,
                     const FeatureValueBuffer& featureBuffer) {
    if (iconStyle && iconStyle->IsVisible() && !iconStyle->IsOverlay()) {
      osmscout::Vertex2D screenPos;
      projection.GeoToPixel(coord,
                            screenPos);
      QRectF iconRect;

      if (!iconStyle->GetIconName().empty()) {
        iconRect=QRectF(screenPos.GetX() - iconSize/2,
                        screenPos.GetY()-iconSize/2,
                        iconSize,
                        iconSize);
      } else {
        auto symbol=iconStyle->GetSymbol();
        assert(symbol);
        double w=symbol->GetWidth(projection);
        double h=symbol->GetHeight(projection);
        iconRect=QRectF(screenPos.GetX() - w/2,
                        screenPos.GetY()-h/2,
                        w,
                        h);
      }
      if (iconRect.intersects(tapRectangle)) {
        double distanceSquare=iconRect.contains(lookupCoord) ? 0 :
          std::pow(lookupCoord.x()-screenPos.GetX(),2)+std::pow(lookupCoord.y()-screenPos.GetY(),2);

        QString name;
        QString altName;
        QString ref;
        QString operatorName;
        QString phone;
        QString website;
        QString openingHours;
        if (const osmscout::NameFeatureValue *nameValue=featureBuffer.findValue<osmscout::NameFeatureValue>();
            nameValue!=nullptr){
          name=QString::fromStdString(nameValue->GetLabel(Locale(), 0));
        }
        if (const osmscout::NameAltFeatureValue *altNameValue=featureBuffer.findValue<osmscout::NameAltFeatureValue>();
            altNameValue != nullptr){
          altName=QString::fromStdString(altNameValue->GetLabel(Locale(), 0));
        }
        if (const osmscout::RefFeatureValue *refValue=featureBuffer.findValue<osmscout::RefFeatureValue>();
            refValue != nullptr){
          ref=QString::fromStdString(refValue->GetLabel(Locale(), 0));
        }
        if (const osmscout::OperatorFeatureValue *operatorValue=featureBuffer.findValue<osmscout::OperatorFeatureValue>();
            operatorValue != nullptr){
          operatorName=QString::fromStdString(operatorValue->GetLabel(Locale(), 0));
        }
        if (const osmscout::PhoneFeatureValue *phoneValue=featureBuffer.findValue<osmscout::PhoneFeatureValue>();
            phoneValue!=nullptr){
          phone=QString::fromStdString(phoneValue->GetPhone());
        }
        if (const osmscout::WebsiteFeatureValue *websiteValue=featureBuffer.findValue<osmscout::WebsiteFeatureValue>();
            websiteValue!=nullptr){
          website=QString::fromStdString(websiteValue->GetWebsite());
        }
        if (const osmscout::OpeningHoursFeatureValue *openingHoursValue=featureBuffer.findValue<osmscout::OpeningHoursFeatureValue>();
            openingHoursValue!=nullptr){
          openingHours=QString::fromStdString(openingHoursValue->GetValue());
        }

        findIcons.push_back(MapIcon{QPoint(screenPos.GetX(),screenPos.GetY()),
                                    iconRect, coord, distanceSquare, iconStyle,
                                    databasePath, objectRef, poiId, QString::fromStdString(type->GetName()),
                                    name, altName, ref, operatorName, phone, website, openingHours, QImage()});
      }
    }
  };

  auto VisitArea=[&](const AreaRef a, int poiId) {
    a->VisitRings([&](size_t, const Area::Ring&r, const TypeInfoRef& type) -> bool {
      if (!type->GetIgnore()) {
        auto iconStyle = styleConfig->GetAreaIconStyle(type, r.GetFeatureValueBuffer(), projection);
        auto coord = r.center.value_or(r.GetBoundingBox().GetCenter());
        CheckIcon(iconStyle, coord, a->GetObjectFileRef(), poiId, type, r.GetFeatureValueBuffer());
      }
      return true;
    });
  };

  for (auto const &n:data.nodes) {
    auto iconStyle = styleConfig->GetNodeIconStyle(n->GetFeatureValueBuffer(), projection);
    CheckIcon(iconStyle,
              n->GetCoords(),
              n->GetObjectFileRef(),
              0,
              n->GetFeatureValueBuffer().GetType(),
              n->GetFeatureValueBuffer());
  }
  for (auto const &a:data.areas) {
    VisitArea(a, 0);
  }

  for (auto const &o:overlayObjects){
    if (o.second->getObjectType()==osmscout::RefType::refArea){
      OverlayArea *oa=dynamic_cast<OverlayArea*>(o.second.get());
      if (oa != nullptr) {
        osmscout::AreaRef a = std::make_shared<osmscout::Area>();
        if (oa->toArea(a, *typeConfig)) {
          VisitArea(a, o.first);
        }
      }
    } else if (o.second->getObjectType()==osmscout::RefType::refNode){
      OverlayNode *oo=dynamic_cast<OverlayNode*>(o.second.get());
      if (oo != nullptr) {
        osmscout::NodeRef n = std::make_shared<osmscout::Node>();
        if (oo->toNode(n, *typeConfig)) {
          auto iconStyle = styleConfig->GetNodeIconStyle(n->GetFeatureValueBuffer(), projection);
          CheckIcon(iconStyle,
                    n->GetCoords(),
                    n->GetObjectFileRef(),
                    o.first,
                    n->GetFeatureValueBuffer().GetType(),
                    n->GetFeatureValueBuffer());
        }
      }
    }
  }
  overlayObjects.clear();
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
    if (a.distanceSquare != b.distanceSquare) {
      return a.distanceSquare < b.distanceSquare;
    }
    if (a.iconStyle->GetPriority() != b.iconStyle->GetPriority()) {
      return a.iconStyle->GetPriority() < b.iconStyle->GetPriority();
    }
    return a.screenCoord.x() < b.screenCoord.x();
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
      SymbolRef symbol=findIcon.iconStyle->GetSymbol();
      double margin=symbol->GetMaxBorderWidth(projection)+1;
      findIcon.image = QImage((findIcon.dimensions.width()+margin)*iconImageUpscale,
                              (findIcon.dimensions.height()+margin)*iconImageUpscale,
                              QImage::Format_ARGB32);
      findIcon.image.fill(Qt::transparent);

      QPainter painter(&findIcon.image);
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setRenderHint(QPainter::TextAntialiasing, true);
      SymbolRendererQt renderer(&painter);

      renderer.Render(projection,
                      *symbol,
                      Vertex2D(double(findIcon.image.width()) / 2.0,
                               double(findIcon.image.height()) / 2.0),
                      iconImageUpscale);
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

  overlayObjects=overlayObjectMap;

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
