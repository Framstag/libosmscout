/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Util.h>

#include <cstdio>
#include <cstdlib>

#if defined(__WIN32__) || defined(WIN32)
  #define _USE_MATH_DEFINES
  #include <math.h>
#endif

#include <cmath>
#include <iomanip>

#include <osmscout/Private/Config.h>

#if defined(HAVE_SYS_TIME_H)
  #include <sys/time.h>
#endif

namespace osmscout {

#if defined(__WIN32__) || defined(WIN32)
#else
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

  struct StopClock::StopClockPIMPL
  {
#if defined(HAVE_SYS_TIME_H)
    timeval start;
    timeval stop;
#endif
  };

  StopClock::StopClock()
    : pimpl(new StopClockPIMPL())
  {
#if defined(HAVE_SYS_TIME_H)
    gettimeofday(&pimpl->start,NULL);
#endif
  }

  StopClock::~StopClock()
  {
    delete pimpl;
  }

  void StopClock::Stop()
  {
#if defined(HAVE_SYS_TIME_H)
    gettimeofday(&pimpl->stop,NULL);
#endif
  }

  std::ostream& operator<<(std::ostream& stream, const StopClock& clock)
  {
#if defined(HAVE_SYS_TIME_H)
    timeval diff;

    timersub(&clock.pimpl->stop,&clock.pimpl->start,&diff);

    stream << diff.tv_sec << "." << std::setw(3) << std::setfill('0') << diff.tv_usec/1000;
#else
    stream << "X.XXX";
#endif

    return stream;
  }

  std::string StopClock::ResultString() const
  {
#if defined(HAVE_SYS_TIME_H)
    timeval     diff;
    std::string result;
    std::string seconds;
    std::string millis;

    timersub(&pimpl->stop,&pimpl->start,&diff);

    seconds=NumberToString(diff.tv_sec);
    millis=NumberToString(diff.tv_usec/1000);

    result=seconds;
    result+=".";

    for (size_t i=millis.length()+1; i<=3; i++) {
      result+="0";
    }

    result+=millis;

    return result;
#else
    return "X.XXX";
#endif
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

  bool DecodeNumber(const char* buffer, uint32_t& number, size_t& bytes)
  {
    number=0;
    bytes=1;

    if (buffer[0]==0) {
      return true;
    }
    else {
      size_t idx=0;

      while (true) {
        uint32_t add=(buffer[idx] & 0x7f) << (idx*7);

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

  bool GetDigitValue(char digit, size_t& result)
  {
    switch (digit) {
    case '1':
      result=1;
      return true;
    case '2':
      result=2;
      return true;
    case '3':
      result=3;
      return true;
    case '4':
      result=4;
      return true;
    case '5':
      result=5;
      return true;
    case '6':
      result=6;
      return true;
    case '7':
      result=7;
      return true;
    case '8':
      result=8;
      return true;
    case '9':
      result=9;
      return true;
    case '0':
      result=0;
      return true;
    case 'a':
    case 'A':
      result=10;
      return true;
    case 'b':
    case 'B':
      result=11;
      return true;
    case 'c':
    case 'C':
      result=12;
      return true;
    case 'd':
    case 'D':
      result=13;
      return true;
    case 'e':
    case 'E':
      result=14;
      return true;
    case 'f':
    case 'F':
      result=15;
      return true;
    default:
      return false;
    }
  }

  std::string AppendFileToDir(const std::string& dir, const std::string& file)
  {
#if defined(__WIN32__) || defined(WIN32)
    std::string result(dir);

    if (result.length()>0 && result[result.length()-1]!='\\') {
      result.append("\\");
    }

    result.append(file);

    return result;
#else
    std::string result(dir);

    if (result.length()>0 && result[result.length()-1]!='/') {
      result.append("/");
    }

    result.append(file);

    return result;
#endif
  }

  bool StringToNumber(const char* string, double& value)
  {
    std::istringstream stream(string);

    stream.imbue(std::locale("C"));

    stream >> value;

    return stream.eof();
  }

  bool StringToNumber(const std::string& string, double& value)
  {
    std::istringstream stream(string);

    stream.imbue(std::locale("C"));

    stream >> value;

    return stream.eof();
  }

  std::string StringListToString(const std::list<std::string>& list,
                                 const std::string& separator)
  {
    std::string result;

    for (std::list<std::string>::const_iterator element=list.begin();
        element!=list.end();
        ++element) {
      if (element==list.begin()) {
       result.append(*element);
      }
      else {
        result.append(separator);
        result.append(*element);
      }
    }

    return result;
  }

  double Log2(double x)
  {
    return log(x)/log(2.0l);
  }

  size_t Pow(size_t a, size_t b)
  {
    if (b==0) {
      return 1;
    }

    size_t res=a;

    while (b>1) {
      res=res*a;
      b--;
    }

    return res;
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

  ScanCell::ScanCell(int x, int y)
  : x(x),
    y(y)
  {
    // no code
  }

  /**
   * This functions does a scan conversion of a line with the given start and end points.
   * This problem is equals to the following problem:
   * Assuming an index that works by referencing lines by linking them to all cells in a cell
   * grid that contain or are crossed by the line. Which cells does the line cross?
   *
   * The given vector for the result data is not cleared on start, to allow multiple calls
   * to this method with different line segments.
   *
   * The algorithm of Bresenham is used together with some checks for special cases.
   */
  void ScanConvertLine(int x1, int y1,
                       int x2, int y2,
                       std::vector<ScanCell>& cells)
  {
    bool steep=std::abs(y2-y1)>std::abs(x2-x1);

    if (steep) {
      std::swap(x1,y1);
      std::swap(x2,y2);
    }

    if (x1>x2) {
      std::swap(x1,x2);
      std::swap(y1,y2);
    }

    int dx=x2-x1;
    int dy=std::abs(y2-y1);
    int error=dx/2;
    int ystep;

    int y=y1;

    if (y1<y2) {
      ystep=1;
    }
    else {
      ystep=-1;
    }

    for (int x=x1; x<=x2; x++) {
      if (steep) {
        cells.push_back(ScanCell(y,x));
      }
      else {
        cells.push_back(ScanCell(x,y));
      }

      error-=dy;

      if (error<0) {
        y+=ystep;
        error+=dx;
      }
    }
  }

  void ScanConvertLine(const std::vector<Point>& points,
                       double xTrans, double cellWidth,
                       double yTrans, double cellHeight,
                       std::vector<ScanCell>& cells)
  {
    assert(points.size()>=2);

    for (size_t i=0; i<points.size()-1; i++) {
      int x1=int((points[i].lon-xTrans)/cellWidth);
      int x2=int((points[i+1].lon-xTrans)/cellWidth);
      int y1=int((points[i].lat-yTrans)/cellHeight);
      int y2=int((points[i+1].lat-yTrans)/cellHeight);

      ScanConvertLine(x1,y1,x2,y2,cells);
    }
  }
}
