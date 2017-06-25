/*
  Srtm - a demo program for libosmscout
  Copyright (C) 2013  Vladimir Vyskocil

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

/*
  Samples :

  > Srtm ~/Documents/SRTM 19.468618 -155.593340
  Open SRTM1 hgt file : /Users/vlad/Documents/SRTM/N19W156.hgt
  Height at (19.4686,-155.593) = 3992 m

  > Srtm ~/Documents/SRTM -21.0773 55.4230
  Open SRTM3 hgt file : /Users/vlad/Documents/SRTM/S22E055.hgt
  Height at (-21.0773,55.423) = 1426 m

 > Srtm ~/Documents/SRTM -21.0773 65.4230
 No data for (-21.0773,65.423)

 */

#include <cstring>
#include <iostream>

#include <osmscout/SRTM.h>

int main(int argc, char* argv[])
{

  if (argc!=4) {
    std::cout << "Srtm <SRTM directory> <latitude> <longitude>" << std::endl;
    return 1;
  }

  char *srtmDir = strdup(argv[1]);
  double latitude = atof(argv[2]);
  double longitude = atof(argv[3]);

  osmscout::SRTM srtm(srtmDir);
  int h = srtm.heightAtLocation(latitude, longitude);
  if(h!=osmscout::SRTM::nodata){
    std::cout<<"Height at ("<<latitude<<","<<longitude<<") = "<<h<<" m"<<std::endl;
  } else {
    std::cout<<"No data for ("<<latitude<<","<<longitude<<")"<<std::endl;
  }

  return 0;
}
