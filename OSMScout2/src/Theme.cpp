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

#include "Theme.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QScreen>

#include <iostream>
Theme::Theme()
    : averageCharWidth(0),
      numberCharWidth(0)
{
  // no code
}

Theme::~Theme()
{
  // no code
}

qreal Theme::mmToPixel(qreal mm) const
{
    return mm*GetDPI()/25.4;
}

qreal Theme::GetDPI() const
{
    QScreen *srn = QApplication::screens().at(0);
    qreal dotsPerInch = (qreal)srn->physicalDotsPerInch();

    return dotsPerInch;
}

qreal Theme::GetMapButtonWidth() const
{
    return mmToPixel(9.0);
}

qreal Theme::GetMapButtonHeight() const
{
    return mmToPixel(9.0);
}

int Theme::GetMapButtonFontSize() const
{
    return (int)mmToPixel(6.0);
}

qreal Theme::GetMapButtonSpace() const
{
    return mmToPixel(3.0);
}

qreal Theme::GetHorizSpace() const
{
    return mmToPixel(2.0);
}

qreal Theme::GetVertSpace() const
{
    return mmToPixel(2.0);
}

int Theme::GetAverageCharWidth() const
{
    if (averageCharWidth==0) {
        QFont font;
        QFontMetrics metrics(font);

        averageCharWidth=metrics.averageCharWidth();
    }

    return averageCharWidth;
}

int Theme::GetNumberCharWidth() const
{
    if (numberCharWidth==0) {
        QFont font;
        QFontMetrics metrics(font);

        numberCharWidth=std::max(numberCharWidth,metrics.width('-'));
        numberCharWidth=std::max(numberCharWidth,metrics.width(','));
        numberCharWidth=std::max(numberCharWidth,metrics.width('.'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('0'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('1'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('2'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('3'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('4'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('5'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('6'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('7'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('8'));
        numberCharWidth=std::max(numberCharWidth,metrics.width('9'));

        std::cout << numberCharWidth << std::endl;
    }

    return numberCharWidth;
}


