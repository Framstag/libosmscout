/*
 This source is part of the libosmscout library
 Copyright (C) 2019  Lukas Karas

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

#include <osmscout/navigation/SpeedAgent.h>

#include <osmscout/feature/MaxSpeedFeature.h>

#include <osmscout/navigation/PositionAgent.h>

namespace osmscout {

  namespace {
    // Stationary gate of the position-difference fallback.
    //
    // While the vehicle is standing still, the GPS fix jitters around the true position, so
    // every pair of consecutive fixes differs by about the jitter amplitude. The fallback sums
    // those differences, and the sum grows with the number of fixes: jitter of 1 m per 1 s fix
    // keeps reading as 3.6 km/h however long the vehicle stands still.
    //
    // Jitter does not travel - it stays inside its own radius - while real movement accumulates.
    // The gate therefore compares the *net* displacement between the oldest and the newest fix
    // of a window long enough to separate the two: a walker at 3 km/h covers 4.2 m in 5 s, while
    // fix jitter of up to 1 m amplitude stays inside 2 m. Because the floor is a displacement per
    // window and not a distance per fix, the decision does not depend on the fix rate.
    // Known limit: jitter above about 1.5 m amplitude is indistinguishable from a very slow
    // walker, which is why the receiver's own speed, when it reports one, is still preferred.
    const Timestamp::duration stationaryWindow{std::chrono::seconds(5)};
    const Timestamp::duration stationaryMinHistory{std::chrono::seconds(4)};
    constexpr double          stationaryFloorMeters{3.0};
  }

CurrentSpeedMessage::CurrentSpeedMessage(const Timestamp& timestamp,
                                         double speed):
  NavigationMessage(timestamp),
  speed(speed)
{}

MaxAllowedSpeedMessage::MaxAllowedSpeedMessage(const Timestamp& timestamp,
                                               double maxAllowedSpeed,
                                               bool defined):
  NavigationMessage(timestamp),
  maxAllowedSpeed(maxAllowedSpeed),
  defined(defined)
{
}

std::list<NavigationMessageRef> SpeedAgent::Process(const NavigationMessageRef &message)
{
  using namespace std::chrono;
  std::list<NavigationMessageRef> result;

  // compute actual speed
  auto gpsUpdateMsg = dynamic_cast<GPSUpdateMessage*>(message.get());
  if (gpsUpdateMsg &&
      gpsUpdateMsg->horizontalAccuracy < Meters(100)){

    // --- Prefer GPS-reported speed over position-difference computation ---
    // GPS speed (m/s) is more accurate, especially at low speeds and when
    // updates are infrequent. Fall back to position-diff only when GPS
    // does not provide a speed value (currentSpeed < 0).
    if (gpsUpdateMsg->currentSpeed >= 0.0) {
      double speed = gpsUpdateMsg->currentSpeed * 3.6; // m/s → km/h
      // Sanity cap: reject speeds > 200 km/h (GPS glitch / tunnel exit jump)
      if (speed > 200.0) {
        speed = -1.0;
      }
      result.push_back(std::make_shared<CurrentSpeedMessage>(gpsUpdateMsg->timestamp, speed));

      // Clear FIFO when stationary so position-diff fallback (if ever used)
      // does not linger on old movement segments.
      if (gpsUpdateMsg->currentSpeed < 0.5) {
        segmentFifo.clear();
        recentFixes.clear();
      }
    } else {
      // Fallback: compute speed from position differences
      if (lastPosition &&
          (gpsUpdateMsg->timestamp-lastPosition.time) >= seconds(1)){

        // GPS gap > 10s means signal was lost (tunnel, dropout).
        // Reset FIFO to avoid computing bogus speed from the position jump.
        auto gap = gpsUpdateMsg->timestamp - lastPosition.time;
        if (gap > seconds(10)) {
          segmentFifo.clear();
          recentFixes.clear();
        }

        recentFixes.push_back({gpsUpdateMsg->currentPosition, gpsUpdateMsg->timestamp});
        while (recentFixes.size() > 1 &&
               gpsUpdateMsg->timestamp - recentFixes.front().time > stationaryWindow) {
          recentFixes.pop_front();
        }

        auto historyDuration = gpsUpdateMsg->timestamp - recentFixes.front().time;
        auto netDisplacement = GetEllipsoidalDistance(recentFixes.front().coord,
                                                      gpsUpdateMsg->currentPosition);

        if (historyDuration >= stationaryMinHistory &&
            netDisplacement < Meters(stationaryFloorMeters)) {
          // The fixes only jitter around their own position: the segment distances must not be
          // turned into a speed.
          segmentFifo.clear();
          result.push_back(std::make_shared<CurrentSpeedMessage>(gpsUpdateMsg->timestamp, 0.0));
        } else {
          segmentFifo.push_back({GetEllipsoidalDistance(lastPosition.coord,gpsUpdateMsg->currentPosition),
                                 gpsUpdateMsg->timestamp-lastPosition.time});
          Timestamp::duration fifoDuration{Timestamp::duration::zero()};
          Distance fifoDistance;
          for (const auto &s:segmentFifo){
            fifoDuration+=s.duration;
            fifoDistance+=s.distance;
          }
          auto sec=duration_cast<duration<double>>(fifoDuration);
          if (sec.count()>0){
            double speed=(fifoDistance.AsMeter()/sec.count())*3.6;
            // Sanity cap: reject speeds > 200 km/h (GPS glitch / tunnel exit jump)
            if (speed > 200.0) {
              speed = -1.0;
            }
            result.push_back(std::make_shared<CurrentSpeedMessage>(gpsUpdateMsg->timestamp,speed));
          }
          // pop fifo
          while (!segmentFifo.empty() && fifoDuration>seconds(3)){
            fifoDuration-=segmentFifo.front().duration;
            segmentFifo.pop_front();
          }
        }
      }
    }
    lastPosition={gpsUpdateMsg->currentPosition, gpsUpdateMsg->timestamp};
  }

  auto positionMsg = dynamic_cast<osmscout::PositionAgent::PositionMessage *>(message.get());
  if (positionMsg) {

    // obtain max speed for current route object
    const MaxSpeedFeatureValue *maxSpeedValue = nullptr;
    if (positionMsg->position.typeConfig) {
      MaxSpeedFeatureValueReader reader(*(positionMsg->position.typeConfig));
      if (positionMsg->position.way) {
        maxSpeedValue = reader.GetValue(positionMsg->position.way->GetFeatureValueBuffer());
      }
      if (maxSpeedValue == nullptr && positionMsg->position.area) {
        maxSpeedValue = reader.GetValue(positionMsg->position.area->GetFeatureValueBuffer());
      }
    }
    double maxSpeed = -1;
    if (maxSpeedValue) {
      maxSpeed = maxSpeedValue->GetMaxSpeed();
    }
    if (lastReportedMaxSpeed != maxSpeed) {
      result.push_back(std::make_shared<MaxAllowedSpeedMessage>(positionMsg->timestamp,
                                                                maxSpeed, maxSpeed > 0));
      lastReportedMaxSpeed = maxSpeed;
    }

    // report unknown speed in tunnel
    using namespace std::chrono;
    if (positionMsg->position.state == PositionAgent::PositionState::EstimateInTunnel &&
        lastPosition.time < (positionMsg->timestamp - seconds(5))){
      result.push_back(std::make_shared<CurrentSpeedMessage>(positionMsg->timestamp,-1));
    }
  }

  return result;
}

}
