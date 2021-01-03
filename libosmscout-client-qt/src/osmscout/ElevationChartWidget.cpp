/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2021  Lukáš Karas

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

#include <osmscout/ElevationChartWidget.h>
#include <osmscout/OSMScoutQt.h>

namespace osmscout {

ElevationChartWidget::ElevationChartWidget(QQuickItem* parent):
  QQuickPaintedItem(parent)
{
  elevationModule=OSMScoutQt::GetInstance().MakeElevationModule();

  connect(this, &ElevationChartWidget::elevationProfileRequest,
          elevationModule, &ElevationModule::onElevationProfileRequest,
          Qt::QueuedConnection);

  connect(elevationModule, &ElevationModule::loadingFinished,
          this, &ElevationChartWidget::onLoadingFinished,
          Qt::QueuedConnection);

  connect(elevationModule, &ElevationModule::error,
          this, &ElevationChartWidget::onError,
          Qt::QueuedConnection);

  connect(elevationModule, &ElevationModule::elevationProfileAppend,
          this, &ElevationChartWidget::onElevationProfileAppend,
          Qt::QueuedConnection);
}

ElevationChartWidget::~ElevationChartWidget()
{
  if (elevationModule!=nullptr){
    elevationModule->deleteLater();
    elevationModule=nullptr;
  }
}

void ElevationChartWidget::paint(QPainter *painter)
{
  if (points.empty()){
    return;
  }
  assert(painter);
  assert(lowest.has_value());
  assert(highest.has_value());

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::TextAntialiasing, true);

  QRectF painterViewport=painter->viewport();
  QRectF chartRect(painterViewport.width() * 0.1,
                  painterViewport.height() * 0.05,
                  painterViewport.width() * 0.85,
                  painterViewport.height() * 0.9);

  Distance wayLength=points.back().distance;
  qreal distancePixelM = wayLength==Distance::Zero() ? 0 : chartRect.width() / wayLength.AsMeter();

  Distance eleDiff = highest->elevation - lowest->elevation;
  qreal elePixelM = eleDiff==Distance::Zero() ? 0 : chartRect.height() / eleDiff.AsMeter();

  QPainterPath path;
  path.moveTo(chartRect.left(),
              chartRect.bottom() - (points.front().elevation - lowest->elevation).AsMeter() * elePixelM);
  for (const auto &point:points) {
    path.lineTo(chartRect.left() + point.distance.AsMeter() * distancePixelM,
                chartRect.bottom() - (point.elevation - lowest->elevation).AsMeter() * elePixelM);
  }

  QPainterPath gradientArea=path;
  gradientArea.lineTo(chartRect.bottomRight());
  gradientArea.lineTo(chartRect.bottomLeft());

  QLinearGradient gradient(0,0,0, chartRect.height());
  gradient.setColorAt(0.0, gradientTopColor);
  gradient.setColorAt(1.0, gradientBottomColor);
  painter->fillPath(gradientArea, gradient);

  QPen pen;
  pen.setColor(lineColor);
  pen.setWidthF(lineWidth);
  pen.setStyle(Qt::SolidLine);
  pen.setCapStyle(Qt::RoundCap);

  painter->setPen(pen);
  painter->drawPath(path);
}

void ElevationChartWidget::onError(int requestId)
{
  if (this->requestId!=requestId){
    return;
  }
  loading=false;
  emit loadingChanged();
}

void ElevationChartWidget::onElevationProfileAppend(ElevationModule::ElevationPoints batch, int requestId)
{
  if (this->requestId!=requestId){
    return;
  }
  points.insert(points.end(), batch.begin(), batch.end());
  for (const auto point:batch){
    std::cout << point.distance << " \t" << point.elevation.AsMeter() << " m \t" << point.coord.GetDisplayText() << " (" << point.contour->GetType()->GetName() << " " << point.contour->GetFileOffset() << ")" << std::endl;
    if (!lowest.has_value() || lowest->elevation > point.elevation){
      lowest=point;
    }
    if (!highest.has_value() || highest->elevation < point.elevation){
      highest=point;
    }
  }
  update();
}

void ElevationChartWidget::onLoadingFinished(int requestId)
{
  if (this->requestId!=requestId){
    return;
  }
  loading=false;
  emit loadingChanged();
}


QObject* ElevationChartWidget::getWay() const
{
  if (!way) {
    return nullptr;
  }
  return new OverlayWay(*way); // copy, QML takes ownership
}

void ElevationChartWidget::reset()
{
  way.reset();
  points.clear();
  lowest=std::nullopt;
  highest=std::nullopt;
}

void ElevationChartWidget::setWay(QObject* o)
{
  if (o == nullptr){
    reset();
    emit wayChanged();
    return;
  }

  OverlayWay *newWay = dynamic_cast<OverlayWay*>(o);
  if (newWay == nullptr){
    qWarning() << "Cannot cast " << o << " to OverlayWay";
    return;
  }
  reset();
  way=std::make_shared<OverlayWay>(*newWay);
  emit wayChanged();

  if (breaker){
    breaker->Break();
  }
  requestId++;
  loading=true;
  breaker=std::make_shared<ThreadedBreaker>();
  emit elevationProfileRequest(way, requestId, breaker);
  emit loadingChanged();
}

void ElevationChartWidget::setLineColor(const QColor &color)
{
  if (lineColor==color){
    return;
  }
  lineColor=color;
  emit lineColorChanged();
  update();
}

void ElevationChartWidget::setGradientTopColor(const QColor &color)
{
  if (gradientTopColor==color){
    return;
  }
  gradientTopColor=color;
  emit gradientTopColorChanged();
  update();
}

void ElevationChartWidget::setGradientBottomColor(const QColor &color)
{
  if (gradientBottomColor==color){
    return;
  }
  gradientBottomColor=color;
  emit gradientBottomColorChanged();
  update();
}

void ElevationChartWidget::setLineWidth(qreal w)
{
  if (lineWidth==w){
    return;
  }
  lineWidth=w;
  emit lineWidthChanged();
  update();
}
}
