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
#include <osmscout/util/String.h>

namespace osmscout {

ElevationChartWidget::ElevationChartWidget(QQuickItem* parent):
  QQuickPaintedItem(parent)
{
  OSMScoutQt& osmScoutInst = OSMScoutQt::GetInstance();
  elevationModule=osmScoutInst.MakeElevationModule();

  locale.SetDistanceUnits(osmScoutInst.GetSettings()->GetUnits() == "imperial" ?
                          osmscout::DistanceUnitSystem::Imperial :
                          osmscout::DistanceUnitSystem::Metrics);

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
  if (breaker){
    breaker->Break();
  }

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
  double topMargin = 0;
  double rightMargin = 0;
  double bottomMargin = textPixelSize * 1.5;
  double leftMargin = textPixelSize * 4;
  QRectF chartRect(leftMargin,
                   topMargin,
                  painterViewport.width() - (leftMargin+rightMargin),
                  painterViewport.height() - (topMargin+bottomMargin));

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

  // X axis
  auto distanceIntervals = std::array<int, 10>{500, 250, 200, 100,
                                      50, 25, 20, 10,
                                      5,1};

  std::vector<DistanceUnitPtr> distanceUnits;
  if (locale.GetDistanceUnits() == DistanceUnitSystem::Imperial) {
    distanceUnits.push_back(std::make_shared<Mile>());
    distanceUnits.push_back(std::make_shared<Feet>());
  } else {
    distanceUnits.push_back(std::make_shared<Kilometer>());
    distanceUnits.push_back(std::make_shared<Meter>());
  }

  int distanceLabelInterval=-1;
  DistanceUnitPtr distanceLabelUnit=distanceUnits[0];
  for (size_t ui=0; ui<distanceUnits.size() && distanceLabelInterval < 0; ui++) {
    distanceLabelUnit=distanceUnits[ui];
    for (size_t i = 0; i < distanceIntervals.size() && distanceLabelInterval < 0; i++) {
      if (wayLength.AsMeter() / distanceLabelUnit->Distance(distanceIntervals[i]).AsMeter() > 2) {
        distanceLabelInterval = distanceIntervals[i];
      }
    }
  }
  if (distanceLabelInterval<0){
    distanceLabelInterval=1;
  }

  painter->setPen(textColor);
  QFont font = painter->font();
  font.setPixelSize(textPixelSize);
  painter->setFont(font);

  for (int i = 1; true; i++) {
    double position = distanceLabelUnit->Distance(distanceLabelInterval * i).AsMeter() * distancePixelM;
    if (position > chartRect.width() - 2*textPixelSize) {
      break;
    }
    std::stringstream ss;
    ss << NumberToString(distanceLabelInterval * i, locale);
    ss << locale.GetUnitsSeparator();
    ss << distanceLabelUnit->UnitStr();
    painter->drawText(chartRect.left() + position, chartRect.bottom() + textPixelSize + textPadding, QString::fromStdString(ss.str()));
    if (distancePixelM<=0) {
      break;
    }
  }

  // Y axis
  int eleLabelInterval=-1;
  DistanceUnitPtr eleLabelUnit;
  if (locale.GetDistanceUnits() == DistanceUnitSystem::Imperial){
    eleLabelUnit=std::make_shared<Feet>();
  } else {
    eleLabelUnit=std::make_shared<Meter>();
  }
  for (size_t i = 0; i < distanceIntervals.size() && eleLabelInterval < 0; i++) {
    if (eleDiff.AsMeter() / eleLabelUnit->Distance(distanceIntervals[i]).AsMeter() > 2) {
      eleLabelInterval = distanceIntervals[i];
    }
  }
  if (eleLabelInterval<0){
    eleLabelInterval=1;
  }

  double lowestEleVal = eleLabelUnit->Value(lowest->elevation);
  int firstAxisLabelVal = std::ceil((double)lowestEleVal / (double)eleLabelInterval) * eleLabelInterval;
  double firstPosition = (eleLabelUnit->Distance(firstAxisLabelVal).AsMeter() - lowest->elevation.AsMeter())  * elePixelM;
  // lowest elevation, conditionally
  if (firstPosition > 3*textPixelSize){
    std::stringstream ss;
    ss << NumberToString(lowestEleVal, locale);
    ss << locale.GetUnitsSeparator();
    ss << eleLabelUnit->UnitStr();
    painter->drawText(0, chartRect.bottom() - textPixelSize, chartRect.left()-textPadding, 2*textPixelSize,
                      Qt::AlignVCenter | Qt::AlignRight, QString::fromStdString(ss.str()));
  }

  double lastPosition = firstPosition;
  for (int i=0; true; i++) {
    double position = firstPosition + eleLabelUnit->Distance(eleLabelInterval * i).AsMeter() * elePixelM;
    if (position > chartRect.height() - 1.5*textPixelSize) {
      break;
    }
    lastPosition = position;
    std::stringstream ss;
    ss << NumberToString(firstAxisLabelVal + eleLabelInterval * i, locale);
    ss << locale.GetUnitsSeparator();
    ss << eleLabelUnit->UnitStr();
    painter->drawText(0, chartRect.bottom() - position - textPixelSize, chartRect.left()-textPadding, 2*textPixelSize,
                      Qt::AlignVCenter | Qt::AlignRight, QString::fromStdString(ss.str()));
    if (elePixelM<=0){
      break;
    }
  }

  // highest elevation, conditionally
  if (lastPosition < chartRect.height() - 3*textPixelSize){
    std::stringstream ss;
    ss << NumberToString(eleLabelUnit->Value(highest->elevation), locale);
    ss << locale.GetUnitsSeparator();
    ss << eleLabelUnit->UnitStr();
    painter->drawText(0, chartRect.top() - textPixelSize*0.5, chartRect.left()-textPadding, 2*textPixelSize,
                      Qt::AlignVCenter | Qt::AlignRight, QString::fromStdString(ss.str()));
  }
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
  for (const auto& point:batch){
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

void ElevationChartWidget::setTextColor(const QColor &c)
{
  if (textColor==c){
    return;
  }
  textColor=c;
  emit textColorChanged();
  update();
}

void ElevationChartWidget::setTextPixelSize(int size)
{
  if (textPixelSize==size){
    return;
  }
  textPixelSize=size;
  emit textPixelSizeChanged();
  update();
}

void ElevationChartWidget::setTextPadding(int size)
{
  if (textPadding==size){
    return;
  }
  textPadding=size;
  emit textPaddingChanged();
  update();
}

}
