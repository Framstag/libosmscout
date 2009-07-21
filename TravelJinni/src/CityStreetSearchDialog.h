#ifndef CITYSTREETDIALOG_H
#define CITYSTREETDIALOG_H

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

#include <Lum/Base/L10N.h>
#include <Lum/Base/String.h>

#include <Lum/Model/String.h>
#include <Lum/Model/Table.h>

#include <Lum/Dialog.h>
#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/City.h>

#include "DatabaseTask.h"

/**
  TODO:
  * We should select the first entry if the result list is not empty.
  * We should map key and down in the string field to make an search result
    selectable via keyboard.
 */
class CityStreetSearchDialog : public Lum::Dialog
{
private:
  typedef Lum::Model::StdRefTable<City,std::list<City> > CitiesModel;
  typedef Lum::Base::Reference<CitiesModel>              CitiesModelRef;

  class CitiesDataProvider : public CitiesModel::DataProvider
  {
  public:
    std::wstring GetString(const CitiesModel::Iterator& iter, size_t column) const
    {
      switch (column) {
      case 1:
        return Lum::Base::UTF8ToWString(iter->name);
      default:
        assert(false);
      }
    }
  };

  typedef Lum::Model::StdRefTable<Street,std::list<Street> > StreetsModel;
  typedef Lum::Base::Reference<StreetsModel>                 StreetsModelRef;

  class StreetsDataProvider : public StreetsModel::DataProvider
  {
  public:
    std::wstring GetString(const StreetsModel::Iterator& iter, size_t column) const
    {
      switch (column) {
      case 1:
        return Lum::Base::UTF8ToWString(iter->name);
      default:
        assert(false);
      }
    }
  };

private:
  DatabaseTask*         databaseTask;
  Lum::Model::ActionRef searchStreetAction;
  Lum::Model::ActionRef okAction;
  Lum::Model::StringRef cityName;
  Lum::Model::ActionRef citySearchTimerAction;
  Lum::Model::StringRef streetName;
  Lum::Model::ActionRef streetSearchTimerAction;
  std::list<City>       cities;
  CitiesModelRef        citiesModel;
  Lum::Model::SingleLineSelectionRef citySelection;
  std::list<Street>     streets;
  StreetsModelRef       streetsModel;
  Lum::Model::SingleLineSelectionRef streetSelection;
  bool                  hasResult;
  City                  resultCity;
  Street                resultStreet;

private:
  void FetchCities();
  void FetchStreets();

public:
  CityStreetSearchDialog(DatabaseTask* databaseTask);

  void PreInit();
  void Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg);

  bool HasResult() const;
  const City& GetResultCity() const;
  const Street& GetResultStreet() const;
};

#endif
