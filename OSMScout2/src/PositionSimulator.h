#ifndef LIBOSMSCOUT_POSITIONSIMULATOR_H
#define LIBOSMSCOUT_POSITIONSIMULATOR_H

/*
  OSMScout2 - demo application for libosmscout
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/gpx/GpxFile.h>

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDateTime>

class PositionSimulator: public QObject {
  Q_OBJECT
  Q_PROPERTY(QString track READ getTrack WRITE setTrack)
  Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
  Q_PROPERTY(QDateTime time READ getTime NOTIFY timeChanged)

  Q_PROPERTY(double startLat READ getStartLat NOTIFY startChanged)
  Q_PROPERTY(double startLon READ getStartLon NOTIFY startChanged)
  Q_PROPERTY(double endLat   READ getEndLat   NOTIFY endChanged)
  Q_PROPERTY(double endLon   READ getEndLon   NOTIFY endChanged)

  Q_PROPERTY(double latitude  READ getLat NOTIFY positionChanged)
  Q_PROPERTY(double longitude READ getLon NOTIFY positionChanged)

private:
  std::vector<osmscout::gpx::TrackSegment> segments;
  QString trackFile;
  bool running{false};
  bool fileLoaded{false};
  size_t currentSegment{0};
  size_t currentPoint{0};
  osmscout::GeoCoord currentPosition{osmscout::GeoCoord(0,0)};
  QTimer timer;
  osmscout::Timestamp simulationTime;
  osmscout::gpx::TrackPoint segmentStart{osmscout::GeoCoord(0,0)};
  osmscout::gpx::TrackPoint segmentEnd{osmscout::GeoCoord(0,0)};

signals:
  void positionChanged(double latitude,
                       double longitude,
                       bool horizontalAccuracyValid,
                       double horizontalAccuracy);
  void runningChanged(bool);
  void startChanged(double latitude, double longitude);
  void endChanged(double latitude, double longitude);
  void timeChanged(QDateTime);

private slots:
  void tick();

public:
  PositionSimulator();
  virtual ~PositionSimulator(){}

  QString getTrack() const {
    return trackFile;
  }

  void setTrack(const QString &t);

  bool isRunning() const {
    return running;
  }

  void setRunning(bool);

  double getStartLat() const {
    return segmentStart.coord.GetLat();
  }
  double getStartLon() const {
    return segmentStart.coord.GetLon();
  }
  double getEndLat() const {
    return segmentEnd.coord.GetLat();
  }
  double getEndLon() const {
    return segmentEnd.coord.GetLon();
  }

  double getLat() const {
    return currentPosition.GetLat();
  }
  double getLon() const {
    return currentPosition.GetLon();
  }

  QDateTime getTime() const;

  Q_INVOKABLE void skipTime(uint64_t millis);

private:
  bool setSegment(size_t);
};

#endif //LIBOSMSCOUT_POSITIONSIMULATOR_H
