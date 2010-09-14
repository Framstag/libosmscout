#ifndef OSMSCOUT_UTIL_H
#define OSMSCOUT_UTIL_H

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

#include <stdint.h>

#if defined(__WIN32__) || defined(WIN32)
#else
  #include <sys/time.h>
#endif

#include <cassert>
#include <limits>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/Private/CoreImportExport.h>

#include <osmscout/Point.h>

namespace osmscout {

  /**
    Returns true, if point in area.

    http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm
   */
  /*
  template<typename N, typename M>
  double IsLeft(const N& p0, const N& p1, const M& p2)
  {
    if (p2.id==p0.id || p2.id==p1.id) {
      return 0;
    }

    return (p1.lon-p0.lon)*(p2.lat-p0.lat)-(p2.lon-p0.lon)*(p1.lat-p0.lat);
  }

  template<typename N, typename M>
  bool IsPointInArea(const N& point,
                     const std::vector<M>& nodes)
  {
    for (int i=0; i<nodes.size()-1; i++) {
      if (point.id==nodes[i].id) {
        return true;
      }
    }

    int wn=0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<nodes.size()-1; i++) {   // edge from V[i] to V[i+1]
      if (nodes[i].lat<=point.lat) {         // start y <= P.y
        if (nodes[i+1].lat>point.lat) {     // an upward crossing
          if (IsLeft(nodes[i],nodes[i+1],point) > 0) { // P left of edge
            ++wn;            // have a valid up intersect
          }
        }
      }
      else {                       // start y > P.y (no test needed)
        if (nodes[i+1].lat<=point.lat) {    // a downward crossing
          if (IsLeft(nodes[i],nodes[i+1],point) < 0) { // P right of edge
            --wn;            // have a valid down intersect
          }
        }
      }
    }

    return wn!=0;
  }*/

  /**
    Returns true, if point in area.

    See http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    */
  template<typename N, typename M>
  bool IsPointInArea(const N& point,
                     const std::vector<M>& nodes)
  {
    int  i,j;
    bool c=false;

    for (i=0, j=nodes.size()-1; i<(int)nodes.size(); j=i++) {
      if (point.id==nodes[i].id) {
        return true;
      }

      if ((nodes[i].lat>point.lat)!=(nodes[j].lat>point.lat) &&
          (point.lon<(nodes[j].lon-nodes[i].lon)*(point.lat-nodes[i].lat) /
           (nodes[j].lat-nodes[i].lat)+nodes[i].lon))  {
        c=!c;
      }
    }

    return c;
  }

  /**
    Return true, if area a is in area b
    */
  template<typename N,typename M>
  bool IsAreaInArea(const std::vector<N>& a,
                    const std::vector<M>& b)
  {
    for (typename std::vector<N>::const_iterator i=a.begin(); i!=a.end(); i++) {
      if (!IsPointInArea(*i,b)) {
        return false;
      }
    }

    return true;
  }

  class OSMSCOUT_API NumberSet
  {
    typedef unsigned long Number;

    struct Data
    {
      virtual ~Data();
    };

    struct Refs : public Data
    {
      Data* refs[256];

      Refs();
      ~Refs();
    };

    struct Leaf : public Data
    {
      unsigned char values[32];

      Leaf();
    };

  private:
    Refs refs;

  public:
    NumberSet();
    ~NumberSet();
    void Insert(Number value);
    bool IsSet(Number value) const;
  };

  class OSMSCOUT_API StopClock
  {
  private:
#if defined(__WIN32__) || defined(WIN32)
#else
    timeval start;
    timeval stop;
#endif
  public:
    StopClock();

    void Stop();

    std::string ResultString() const;

    friend OSMSCOUT_API std::ostream& operator<<(std::ostream& stream, const StopClock& clock);
  };

  extern OSMSCOUT_API std::ostream& operator<<(std::ostream& stream, const StopClock& clock);

  extern OSMSCOUT_API void GetKeysForName(const std::string& name, std::set<uint32_t>& keys);

  extern OSMSCOUT_API bool EncodeNumber(unsigned long number,
                                        size_t bufferLength,
                                        char* buffer,
                                        size_t& bytes);
  extern OSMSCOUT_API bool DecodeNumber(const char* buffer, uint32_t& number, size_t& bytes);

  extern OSMSCOUT_API bool GetFileSize(const std::string& filename, long& size);

  extern OSMSCOUT_API bool GetDigitValue(char digit, size_t& result);

  extern OSMSCOUT_API std::string AppendFileToDir(const std::string& dir, const std::string& file);

  template<typename A>
  size_t NumberDigits(const A& a,size_t base=10)
  {
    A      value(a);
    size_t res=0;

    if (value<0) {
      res++;
    }

    while (value!=0) {
      res++;
      value=value/base;
    }

    return res;
  }

  template<typename A>
  std::string NumberToString(const A& a)
  {
    std::string res;
    A           value(a);
    bool        negative=false;

    if (std::numeric_limits<A>::is_signed) {
      if (value<0) {
        negative=true;
        value=-value;
      }
    }

    res.reserve(20);

    while (value!=0) {
      res.insert(0,1,'0'+value%10);
      value=value/10;
    }

    if (res.empty()) {
      res.insert(0,1,'0');
    }

    if (negative) {
      res.insert(0,1,'-');
    }

    return res;
  }

  extern OSMSCOUT_API bool StringToNumber(const char* string, double& value);
  extern OSMSCOUT_API bool StringToNumber(const std::string& string, double& value);

  template<typename A>
  bool StringToNumber(const std::string& string, A& a, size_t base=10)
  {
    assert(base<=16);

    std::string::size_type pos=0;
    bool                   minus=false;

    a=0;

    if (string.empty()) {
      return false;
    }

    if (!std::numeric_limits<A>::is_signed && string[0]=='-') {
      return false;
    }

    /*
      Special handling for the first symbol/digit (could be negative)
      */
    if (base==10 && string[0]=='-') {
      minus=true;
      pos=1;
    }
    else {
      size_t digitValue;

      if (!GetDigitValue(string[pos],digitValue)) {
        return false;
      }

      if (digitValue>=base) {
        return false;
      }

      /*
        For signed values with base!=10 we assume a negative value
      */
      if (digitValue==base-1 &&
          std::numeric_limits<A>::is_signed &&
          string.length()==NumberDigits(std::numeric_limits<A>::max())) {
        minus=true;
        a=base/2;
      }
      else {
        a=digitValue;
      }

      pos=1;
    }

    while (pos<string.length()) {
      size_t digitValue;

      if (!GetDigitValue(string[pos],digitValue)) {
        return false;
      }

      if (digitValue>=base) {
        return false;
      }

      if (std::numeric_limits<A>::max()/base-(A)digitValue<a) {
        return false;
      }

      a=a*base+digitValue;

      pos++;
    }

    if (minus) {
      a=-a;
    }

    return true;
  }

  extern OSMSCOUT_API std::string StringListToString(const std::list<std::string>& list,
                                                     const std::string& separator="/");

  extern OSMSCOUT_API double Log2(double x);
  extern OSMSCOUT_API size_t Pow(size_t a, size_t b);
  extern OSMSCOUT_API double GetSphericalDistance(double aLon, double aLat,
                                                  double bLon, double bLat);
  extern OSMSCOUT_API double GetEllipsoidalDistance(double aLon, double aLat,
                                                   double bLon, double bLat);

  struct OSMSCOUT_API ScanCell
  {
    int x;
    int y;

    ScanCell(int x, int y);
  };

  void OSMSCOUT_API ScanConvertLine(int x1, int y1,
                                    int x2, int y2,
                                    std::vector<ScanCell>& cells);
  void OSMSCOUT_API ScanConvertLine(const std::vector<Point>& points,
                                    double xTrans, double cellWidth,
                                    double yTrans, double cellHeight,
                                    std::vector<ScanCell>& cells);
}

#endif
