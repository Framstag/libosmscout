#ifndef OSMSCOUT_CLIENT_QT_ELEVATIONCHARTWIDGET_H
#define OSMSCOUT_CLIENT_QT_ELEVATIONCHARTWIDGET_H

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

#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/OverlayObject.h>
#include <osmscoutclientqt/ElevationModule.h>
#include <osmscout/util/Locale.h>

#include <QQuickPaintedItem>
#include <QColor>

namespace osmscout {

class OSMSCOUT_CLIENT_QT_API ElevationChartWidget : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QObject *way READ getWay WRITE setWay NOTIFY wayChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QColor lineColor READ getLineColor WRITE setLineColor NOTIFY lineColorChanged)
  Q_PROPERTY(qreal lineWidth READ getLineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
  Q_PROPERTY(QColor gradientTopColor READ getGradientTopColor WRITE setGradientTopColor NOTIFY gradientTopColorChanged)
  Q_PROPERTY(QColor gradientBottomColor READ getGradientBottomColor WRITE setGradientBottomColor NOTIFY gradientBottomColorChanged)
  Q_PROPERTY(QColor textColor READ getTextColor WRITE setTextColor NOTIFY textColorChanged)
  Q_PROPERTY(int textPixelSize READ getTextPixelSize WRITE setTextPixelSize NOTIFY textPixelSizeChanged)
  Q_PROPERTY(int textPadding READ getTextPadding WRITE setTextPadding NOTIFY textPaddingChanged)

  Q_PROPERTY(qint64 pointCount READ getPointCount NOTIFY pointsUpdated)
  Q_PROPERTY(double lowestElevation READ getLowestElevation NOTIFY pointsUpdated)
  Q_PROPERTY(double highestElevation READ getHighestElevation NOTIFY pointsUpdated)
  Q_PROPERTY(double ascent READ getAscent NOTIFY pointsUpdated)
  Q_PROPERTY(double descent READ getDescent NOTIFY pointsUpdated)

signals:
  void wayChanged();
  void loadingChanged();
  void elevationProfileRequest(std::shared_ptr<OverlayWay> way,
                               int requestId,
                               osmscout::BreakerRef breaker);
  void lineColorChanged();
  void lineWidthChanged();
  void gradientTopColorChanged();
  void gradientBottomColorChanged();
  void textColorChanged();
  void textPixelSizeChanged();
  void textPaddingChanged();
  void pointsUpdated();

public slots:
  void onError(int requestId);
  void onElevationProfileAppend(ElevationModule::ElevationPoints points, int requestId);
  void onLoadingFinished(int requestId);

public:
  explicit ElevationChartWidget(QQuickItem* parent = nullptr);
  ~ElevationChartWidget() override;

  void paint(QPainter *painter) override;

  QObject* getWay() const;
  void setWay(QObject* o);

  bool isLoading() const
  {
    return loading;
  }

  QColor getLineColor() const
  {
    return lineColor;
  }

  void setLineColor(const QColor &color);

  qreal getLineWidth() const
  {
    return lineWidth;
  }

  void setLineWidth(qreal w);

  QColor getGradientTopColor() const
  {
    return gradientTopColor;
  }

  void setGradientTopColor(const QColor &c);

  QColor getGradientBottomColor() const
  {
    return gradientBottomColor;
  }

  void setGradientBottomColor(const QColor &c);

  QColor getTextColor() const
  {
    return textColor;
  }

  void setTextColor(const QColor &c);

  int getTextPixelSize() const
  {
    return textPixelSize;
  }

  void setTextPixelSize(int size);

  int getTextPadding() const
  {
    return textPadding;
  }

  void setTextPadding(int size);

  qint64 getPointCount() const
  {
    return points.size();
  }

  double getLowestElevation() const
  {
    if (lowest.has_value()) {
      return lowest->elevation.AsMeter();
    }
    return 0;
  }

  double getHighestElevation() const
  {
    if (highest.has_value()) {
      return highest->elevation.AsMeter();
    }
    return 0;
  }

  double getAscent() const
  {
    return ascent.AsMeter();
  }

  double getDescent() const
  {
    return descent.AsMeter();
  }

protected:
  void reset();

protected:
  ElevationModule* elevationModule=nullptr;
  std::shared_ptr<OverlayWay> way;
  osmscout::BreakerRef breaker;
  bool loading=false;
  int requestId=0;

  ElevationModule::ElevationPoints points;
  std::optional<ElevationPoint> lowest;
  std::optional<ElevationPoint> highest;
  osmscout::Distance ascent;
  osmscout::Distance descent;

  QColor lineColor=Qt::darkBlue;
  QColor textColor=Qt::darkBlue;
  QColor gradientTopColor=QColor(lineColor.red(), lineColor.green(), lineColor.blue(), 0xA0);
  QColor gradientBottomColor=Qt::transparent;
  qreal lineWidth=5;
  int textPixelSize=14;
  int textPadding=4;

  Locale locale=Locale::ByEnvironmentSafe();
};

}

#endif // OSMSCOUT_CLIENT_QT_ELEVATIONCHARTWIDGET_H
