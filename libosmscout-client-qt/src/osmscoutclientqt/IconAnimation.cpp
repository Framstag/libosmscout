/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2022  Lukáš Karas

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

#include <osmscoutclientqt/IconAnimation.h>

#include <cmath>

namespace osmscout {

IconAnimation::IconAnimation():
  QObject(nullptr)
{
  timer.setSingleShot(false);
  timer.setInterval(16);

  connect(&timer, &QTimer::timeout, this, &IconAnimation::tick);
}

void IconAnimation::activate(const MapIcon &icon)
{
  QElapsedTimer t;
  t.start();
  icons.push_back(Animation{icon, icon.dimensions.width(), icon.dimensions.width(), State::FadeIn, t});
  if (!timer.isActive()) {
    timer.start();
  }
  emit update();
}

void IconAnimation::deactivateAll()
{
  for (Animation &animation: icons) {
    if (animation.state != State::FadeOut) {
      animation.startSize=animation.size;
      animation.state = State::FadeOut;
      animation.duration.restart();
    }
  }
  if (!timer.isActive() && !icons.empty()) {
    timer.start();
  }
  emit update();
}

void IconAnimation::paint(QPainter *painter, const MercatorProjection &projection)
{
  for (Animation &animation: icons) {
    double x,y;
    projection.GeoToPixel(animation.icon.coord, x, y);
    double w = animation.size;
    double h = animation.icon.image.height() * (w / animation.icon.image.width());

    // draw semitransparent black circle as background
    double r = std::sqrt(std::pow(w, 2) + std::pow(h, 2)) / 2;
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor::fromRgbF(0, 0, 0, 0.1)));
    painter->drawEllipse(QPointF(x, y) ,r ,r);

    painter->drawImage(QRectF(x-w/2, y-h/2, w, h), animation.icon.image);
  }
}

void IconAnimation::tick()
{
  int running=0;
  for (auto it=icons.begin(); it!=icons.end();) {
    Animation &animation=*it;
    if (animation.state==State::FadeIn) {
      if (animation.duration.elapsed() >= animationDuration) {
        animation.state=State::Full;
        animation.size=animation.MaxSize();
        it++;
      } else {
        double progress = double(animation.duration.elapsed()) / double(animationDuration);
        animation.size = animation.startSize + (animation.MaxSize() - animation.startSize) * progress;
        running++;
        it++;
      }
    } else if (animation.state==State::FadeOut){
      if (animation.duration.elapsed() >= animationDuration) {
        it=icons.erase(it);
      } else {
        double progress = double(animation.duration.elapsed()) / double(animationDuration);
        animation.size = animation.startSize - (animation.startSize - animation.MinSize()) * progress;
        running++;
        it++;
      }
    } else {
      it++;
    }
  }
  if (running==0) {
    timer.stop();
  }
  emit update();
}

}

