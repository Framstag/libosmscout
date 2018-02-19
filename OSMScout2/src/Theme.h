#ifndef THEME_H
#define THEME_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2014  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <QSettings>

class Theme : public QObject
{
  Q_OBJECT

  Q_PROPERTY(qreal dpi READ GetDPI)

    Q_PROPERTY(int textFontSize READ GetTextFontSize CONSTANT)

  // Attributes of overlay buttons on map screen
  Q_PROPERTY(qreal mapButtonWidth READ GetMapButtonWidth CONSTANT)
  Q_PROPERTY(qreal mapButtonHeight READ GetMapButtonHeight CONSTANT)
  Q_PROPERTY(int mapButtonFontSize READ GetMapButtonFontSize CONSTANT)
  Q_PROPERTY(qreal mapButtonSpace READ GetMapButtonSpace CONSTANT)

  // Common distances between GUI elements
  Q_PROPERTY(qreal horizSpace READ GetHorizSpace CONSTANT)
  Q_PROPERTY(qreal vertSpace READ GetVertSpace CONSTANT)

  Q_PROPERTY(int averageCharWidth READ GetAverageCharWidth CONSTANT)
  Q_PROPERTY(int numberCharWidth READ GetNumberCharWidth CONSTANT)

private:
    qreal mmToPixel(qreal mm) const;
    qreal pointToPixel(qreal point) const;

    mutable int textFontSize;
    mutable int averageCharWidth;
    mutable int numberCharWidth;

public:
    Theme();
    ~Theme() override;

    qreal GetDPI() const;

    int GetTextFontSize() const;

    qreal GetMapButtonWidth() const;
    qreal GetMapButtonHeight() const;
    int GetMapButtonFontSize() const;
    qreal GetMapButtonSpace() const;

    qreal GetHorizSpace() const;
    qreal GetVertSpace() const;

    int GetAverageCharWidth() const;
    int GetNumberCharWidth() const;
};

#endif
