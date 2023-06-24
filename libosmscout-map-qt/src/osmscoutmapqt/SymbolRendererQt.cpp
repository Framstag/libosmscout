/*
  This source is part of the libosmscout-map library
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2022  Lukas Karas

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

#include <osmscout/log/Logger.h>
#include <osmscoutmapqt/SymbolRendererQt.h>

#include <QPainterPath>

namespace osmscout {
SymbolRendererQt::SymbolRendererQt(QPainter *painter):
  painter(painter)
{}

void SymbolRendererQt::SetFill(const FillStyleRef &fillStyle)
{
  if (fillStyle) {
    if (fillStyle->HasPattern()) {
      log.Warn() << "Pattern is not supported for symbols";
    }
    if (fillStyle->GetFillColor().IsVisible()) {
      painter->setBrush(QBrush(QColor::fromRgbF(fillStyle->GetFillColor().GetR(),
                                                fillStyle->GetFillColor().GetG(),
                                                fillStyle->GetFillColor().GetB(),
                                                fillStyle->GetFillColor().GetA())));
    } else {
      painter->setBrush(Qt::NoBrush);
    }
  } else {
    painter->setBrush(Qt::NoBrush);
  }
}

void SymbolRendererQt::SetBorder(const BorderStyleRef &borderStyle, double screenMmInPixel)
{
  if (borderStyle) {
    double borderWidth=borderStyle->GetWidth() * screenMmInPixel;

    if (borderWidth>=0) {
      QPen pen;

      pen.setColor(QColor::fromRgbF(borderStyle->GetColor().GetR(),
                                    borderStyle->GetColor().GetG(),
                                    borderStyle->GetColor().GetB(),
                                    borderStyle->GetColor().GetA()));
      pen.setWidthF(borderWidth);

      if (!borderStyle->HasDashes()) {
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap);
      }
      else {
        QVector<qreal> dashes;

        for (double i : borderStyle->GetDash()) {
          dashes << i;
        }

        pen.setDashPattern(dashes);
        pen.setCapStyle(Qt::FlatCap);
      }

      painter->setPen(pen);
    }
    else {
      painter->setPen(Qt::NoPen);
    }
  } else {
    painter->setPen(Qt::NoPen);
  }
}

void SymbolRendererQt::DrawPolygon(const std::vector<Vertex2D> &polygonPixels)
{
  QPainterPath path;

  for (auto pixel=polygonPixels.begin();
       pixel!=polygonPixels.end();
       ++pixel) {
    if (pixel==polygonPixels.begin()) {
      path.moveTo(pixel->GetX(), pixel->GetY());
    } else {
      path.lineTo(pixel->GetX(), pixel->GetY());
    }
  }
  painter->drawPath(path);
}

void SymbolRendererQt::DrawRect(double x, double y, double w, double h)
{
  QPainterPath path;

  path.addRect(x, y, w, h);
  painter->drawPath(path);
}

void SymbolRendererQt::DrawCircle(double x, double y, double radius)
{
    QPainterPath path;

    path.addEllipse(QPointF(x,y), radius, radius);
    painter->drawPath(path);
}

}
