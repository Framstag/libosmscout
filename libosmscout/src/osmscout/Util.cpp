/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Util.h>

#include <cstdio>
#include <cmath>
#include <iomanip>
#include <iostream>

#ifndef timersub
    # define timersub(a, b, result) \
          do { \
                (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
                (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
                if ((result)->tv_usec < 0) { \
                      --(result)->tv_sec; \
                  (result)->tv_usec += 1000000; \
                } \
          } while (0)
#endif

NumberSet::Data::~Data()
{
  // no code
}

NumberSet::Refs::Refs()
{
  for (size_t i=0; i<256; i++) {
    refs[i]=NULL;
  }
}

NumberSet::Refs::~Refs()
{
  for (size_t i=0; i<256; i++) {
    delete refs[i];
  }
}

NumberSet::Leaf::Leaf()
{
  for (size_t i=0; i<32; i++) {
    values[i]=0;
  }
}

NumberSet::NumberSet()
{
  // no code
}

NumberSet::~NumberSet()
{
  // no code
}

void NumberSet::Insert(Number value)
{
  unsigned char byte;
  Refs          *r=&refs;

  byte=value >> (sizeof(Number)-1)*8;

  if (r->refs[byte]==NULL) {
    r->refs[byte]=new Refs();
  }

  for (size_t i=2; i<sizeof(Number)-1; i++) {
    r=static_cast<Refs*>(r->refs[byte]);

    byte=value >> (sizeof(Number)-i)*8;

    if (r->refs[byte]==NULL) {
      r->refs[byte]=new Refs();
    }
  }

  r=static_cast<Refs*>(r->refs[byte]);

  byte=value >> 1*8;

  if (r->refs[byte]==NULL) {
    r->refs[byte]=new Leaf();
  }

  Leaf *l=static_cast<Leaf*>(r->refs[byte]);

  byte=value & 0xff;

  size_t by=byte/32;
  size_t bi=byte%8;

  l->values[by]|=(1 << bi);
}

bool NumberSet::IsSet(Number value) const
{
  unsigned char byte;
  const Refs    *r=&refs;

  byte=value >> (sizeof(Number)-1)*8;

  if (r->refs[byte]==NULL) {
    return false;;
  }

  for (size_t i=2; i<sizeof(Number); i++) {
    r=dynamic_cast<const Refs*>(r->refs[byte]);

    byte=value >> (sizeof(Number)-i)*8;

    if (r->refs[byte]==NULL) {
      return false;
    }
  }

  const Leaf *l=static_cast<const Leaf*>(r->refs[byte]);

  byte=value & 0xff;

  size_t by=byte/32;
  size_t bi=byte%8;

  return l->values[by] & (1 << bi);
}

StopClock::StopClock()
{
  gettimeofday(&start,NULL);
}

void StopClock::Stop()
{
  gettimeofday(&stop,NULL);
}

std::ostream& operator<<(std::ostream& stream, const StopClock& clock)
{
  timeval diff;

  timersub(&clock.stop,&clock.start,&diff);

  stream << diff.tv_sec << "." << std::setw(3) << std::setfill('0') << diff.tv_usec/1000;

  return stream;
}


void GetKeysForName(const std::string& name, std::set<uint32_t>& keys)
{
  for (size_t s=0; s==0 || s+4<=name.length(); s++) {
    uint32_t value=0;

    if (name.length()>s) {
      value=name[s];
    }
    value=value << 8;

    if (name.length()>s+1) {
      value+=name[s+1];
    }
    value=value << 8;

    if (name.length()>s+2) {
      value+=name[s+2];
    }
    value=value << 8;

    if (name.length()>s+3) {
      value+=name[s+3];
    }

    keys.insert(value);
  }
}

bool EncodeNumber(unsigned long number,
                  size_t bufferLength,
                  char* buffer,
                  size_t& bytes)
{
  if (number==0) {
    if (bufferLength==0) {
      return false;
    }

    buffer[0]=0;
    bytes=1;
    return true;
  }
  else {
    bytes=0;

    while (number!=0) {
      char          byte;
      unsigned long rest;

      byte=number & 0xff;
      rest=number >> 8;

      if ((rest!=0 || byte & 0x80)!=0) {
        // If we have something to encode or the high bit is set
        // we need an additional byte and can only decode 7 bit
        byte=byte & 0x7f; // Mask out the lower 7 bytes
        byte=byte| 0x80;  // set the 8th byte to signal that more bytes will follow
        number=number >> 7;

        if (bufferLength==0) {
          return false;
        }

        buffer[bytes]=byte;
        bytes++;
        bufferLength--;
      }
      else {
        if (bufferLength==0) {
          return false;
        }

        number=0; // we are finished!

        buffer[bytes]=byte;
        bytes++;
        bufferLength--;
      }
    }
  }

  return true;
}

bool DecodeNumber(const char* buffer, unsigned long& number, size_t& bytes)
{
  number=0;
  bytes=1;

  if (buffer[0]==0) {
    return true;
  }
  else {
    size_t idx=0;

    while (true) {
      size_t add=(buffer[idx] & 0x7f) << (idx*7);

      number=number | add;

      if ((buffer[idx] & 0x80)==0) {
        return true;
      }

      bytes++;
      idx++;
    };
  }
}

bool GetFileSize(const std::string& filename, long& size)
{
  FILE *file;

  file=fopen(filename.c_str(),"rb");

  if (file==NULL) {
    return false;
  }

  if (fseek(file,0L,SEEK_END)!=0) {
    fclose(file);

    return false;
  }

  size=ftell(file);

  if (size==-1) {
    fclose(file);

    return false;
  }

  fclose(file);

  return true;
}

/**
  Calculating basic cost for the A* algorithm based on the
  spherical distance of two points on earth
  */
double GetSphericalDistance(double aLon, double aLat,
                            double bLon, double bLat)
{
  double r=6371.01; // Average radius of earth
  double dLat=(bLat-aLat)*M_PI/180;
  double dLon=(bLon-aLon)*M_PI/180;

  double a = sin(dLat/2)*sin(dLat/2)+cos(dLon/2)*cos(dLat/2)*sin(dLon/2)*sin(dLon/2);

  double c = 2*atan2(sqrt(a),sqrt(1-a));

  return r*c;
}

/**
  Calculating Vincenty's inverse for getting the ellipsoidal distance
  of two points on earth.
  */
double GetEllipsoidalDistance(double aLon, double aLat,
                              double bLon, double bLat)
{
  double a=6378137;
  double b=6356752.3142;
  double f=1/298.257223563;  // WGS-84 ellipsiod
  double phi1=aLat*M_PI/180;
  double phi2=bLat*M_PI/180;
  double lambda1=aLon*M_PI/180;
  double lambda2=bLon*M_PI/180;
  double a2b2b2=(a*a - b*b) / (b*b);

  double omega=lambda2 - lambda1;

  double U1=atan((1.0 - f) * tan(phi1));
  double sinU1=sin(U1);
  double cosU1=cos(U1);

  double U2=atan((1.0 - f) * tan(phi2));
  double sinU2=sin(U2);
  double cosU2=cos(U2);

  double sinU1sinU2=sinU1 * sinU2;
  double cosU1sinU2=cosU1 * sinU2;
  double sinU1cosU2=sinU1 * cosU2;
  double cosU1cosU2=cosU1 * cosU2;

  double lambda=omega;

  double A=0.0;
  double B=0.0;
  double sigma=0.0;
  double deltasigma=0.0;
  double lambda0;
  bool converged=false;

  for (int i=0; i < 10; i++)
  {
    lambda0=lambda;

    double sinlambda=sin(lambda);
    double coslambda=cos(lambda);

    double sin2sigma=(cosU2 * sinlambda * cosU2 * sinlambda) +
                      (cosU1sinU2 - sinU1cosU2 * coslambda) *
                      (cosU1sinU2 - sinU1cosU2 * coslambda);

    double sinsigma=sqrt(sin2sigma);

    double cossigma=sinU1sinU2 + (cosU1cosU2 * coslambda);

    sigma=atan2(sinsigma, cossigma);

    double sinalpha=(sin2sigma == 0) ? 0.0 :
                     cosU1cosU2 * sinlambda / sinsigma;

    double alpha=asin(sinalpha);
    double cosalpha=cos(alpha);
    double cos2alpha=cosalpha * cosalpha;

    double cos2sigmam=cos2alpha == 0.0 ? 0.0 :
                       cossigma - 2 * sinU1sinU2 / cos2alpha;

    double u2=cos2alpha * a2b2b2;

    double cos2sigmam2=cos2sigmam * cos2sigmam;

    A=1.0 + u2 / 16384 * (4096 + u2 *
                          (-768 + u2 * (320 - 175 * u2)));

    B=u2 / 1024 * (256 + u2 * (-128 + u2 * (74 - 47 * u2)));

    deltasigma=B * sinsigma * (cos2sigmam + B / 4 *
                               (cossigma * (-1 + 2 * cos2sigmam2) - B / 6 *
                                cos2sigmam * (-3 + 4 * sin2sigma) *
                                (-3 + 4 * cos2sigmam2)));

    double C=f / 16 * cos2alpha * (4 + f * (4 - 3 * cos2alpha));

    lambda=omega + (1 - C) * f * sinalpha *
           (sigma + C * sinsigma * (cos2sigmam + C *
                                    cossigma * (-1 + 2 * cos2sigmam2)));

    if ((i > 1) && (std::abs((lambda - lambda0) / lambda) < 0.0000000000001)) {
      converged=true;
      break;
    }
  }

  return b * A * (sigma - deltasigma)/1000; // We want the distance in Km
}

