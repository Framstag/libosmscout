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

#include <osmscout/CityStreetIndex.h>

#include <cassert>
#include <iostream>

#include <osmscout/FileScanner.h>
#include <osmscout/Util.h>

CityStreetIndex::CityStreetIndex()
 : urbanLoaded(false)
{
  // no code
}

CityStreetIndex::~CityStreetIndex()
{
  // no code
}

bool CityStreetIndex::LoadUrban(Id id) const
{
  FileScanner                         scanner;
  std::string                         file=path+"/"+"citystreet.idx";
  std::map<Id,size_t>::const_iterator offset;

  urban.ways.clear();
  urban.areas.clear();
  urbanLoaded=false;

  offset=urbanOffsets.find(id);

  assert(offset!=urbanOffsets.end());

  if (!scanner.Open(file)) {
    return false;
  }

  std::cout << "Loading urban " << id << " " << offset->second << std::endl;

  scanner.SetPos(offset->second);

  urban.id=id;

  size_t wayEntries;
  size_t areaEntries;

  scanner.ReadNumber(wayEntries);  // Number of ways
  scanner.ReadNumber(areaEntries); // Number of areas

  std::cout << wayEntries << " ways, " << areaEntries << " areas" << std::endl;

  for (size_t i=0; i<wayEntries; i++) {
    std::string                                                    name;
    size_t                                                         idEntries;
    std::pair<std::map<std::string,std::list<Id> >::iterator,bool> result;

    scanner.Read(name); // The name of the way

    result=urban.ways.insert(std::pair<std::string,std::list<Id> >(name,std::list<Id>()));

    scanner.ReadNumber(idEntries); // Number of ids

    for (size_t j=0; j<idEntries;j++) {
      Id id;

      scanner.Read(id); // The id

      result.first->second.push_back(id);
    }
  }

  for (size_t i=0; i<areaEntries; i++) {
    std::string                                                    name;
    size_t                                                         idEntries;
    std::pair<std::map<std::string,std::list<Id> >::iterator,bool> result;

    scanner.Read(name); // The name of the way

    result=urban.areas.insert(std::pair<std::string,std::list<Id> >(name,std::list<Id>()));

    scanner.ReadNumber(idEntries); // Number of ids

    for (size_t j=0; j<idEntries;j++) {
      Id id;

      scanner.Read(id); // The id

      result.first->second.push_back(id);
    }
  }

  if (scanner.HasError()) {
    urban.ways.clear();
    urban.areas.clear();
    urbanLoaded=false;
    return false;
  }

  urbanLoaded=true;

  return scanner.Close();
}

bool CityStreetIndex::LoadCityStreetIndex(const std::string& path)
{
  FileScanner   scanner;
  std::string   file=path+"/"+"citystreet.idx";

  this->path=path;

  if (!scanner.Open(file)) {
    return false;
  }

  size_t cityEntries;

  scanner.ReadNumber(cityEntries); // Number of cities

  std::cout << cityEntries << " citys..." << std::endl;

  for (size_t i=0; i<cityEntries; i++) {
    City city;

    Id            id;
    unsigned long refType;

    scanner.Read(id);            // The id of the city
    scanner.ReadNumber(refType); // Type of id
    scanner.Read(city.urbanId);  // The urban id
    scanner.Read(city.name);     // The name of the city

    city.reference.Set(id,(RefType)refType);

    cities.push_back(city);
  }

  size_t urbanEntries;

  scanner.ReadNumber(urbanEntries); // Number of urbans

  std::cout << urbanEntries << " urbans..." << std::endl;

  for (size_t i=0; i<urbanEntries; i++) {
    Id     id;
    size_t offset;

    scanner.Read(id);     // The id of the urban
    scanner.Read(offset); // The file offset for this urban

    urbanOffsets[id]=offset;
  }

  return !scanner.HasError() && scanner.Close();
}


bool CityStreetIndex::GetMatchingCities(const std::string& name,
                                        std::list<City>& cities,
                                        size_t limit,
                                        bool& limitReached) const
{
  cities.clear();
  limitReached=false;

  for (std::list<City>::const_iterator city=this->cities.begin();
       city!=this->cities.end();
       ++city) {
    if (city->name.find(name)!=std::string::npos) {
      if (cities.size()>=limit) {
        std::cout << "Limit reached!!!" << std::endl;
        limitReached=true;
        return true;
      }
      else {
        cities.push_back(*city);
      }
    }
  }

  return true;
}

bool CityStreetIndex::GetMatchingStreets(Id urbanId, const std::string& name,
                                         std::list<Street>& streets,
                                         size_t limit, bool& limitReached) const
{
  streets.clear();
  limitReached=false;

  if (!urbanLoaded || urban.id!=urbanId) {
    if (!LoadUrban(urbanId) || !urbanLoaded) {
      return false;
    }
  }

  for (std::map<std::string,std::list<Id> >::const_iterator way=urban.ways.begin();
       way!=urban.ways.end();
       ++way) {
    if (way->first.find(name)!=std::string::npos) {
      if (streets.size()>=limit) {
        limitReached=true;
        return true;
      }
      else {
        Street street;

        street.reference.Set(*way->second.begin(),refWay);
        street.name=way->first;

        streets.push_back(street);
        //ways.insert(ways.end(),way->second.begin(),way->second.end());
      }
    }
  }

  for (std::map<std::string,std::list<Id> >::const_iterator area=urban.areas.begin();
       area!=urban.areas.end();
       ++area) {
    if (area->first.find(name)!=std::string::npos) {
      if (streets.size()>=limit) {
        limitReached=true;
        return true;
      }
      else {
        Street street;

        street.reference.Set(*area->second.begin(),refArea);
        street.name=area->first;

        streets.push_back(street);
        //areas.insert(areas.end(),area->second.begin(),area->second.end());
      }
    }
  }

  return true;
}

void CityStreetIndex::DumpStatistics()
{
  size_t memory=0;

  memory+=cities.size()*sizeof(City);
  memory+=urbanOffsets.size()*(sizeof(Id)+sizeof(size_t));

  std::cout << "city street size " << cities.size() << ", memory " << memory << std::endl;
}

