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

#include "CityStreetSearchDialog.h"

#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>
#include <Lum/Wizard.h>

#include <osmscout/Database.h>

CityStreetSearchDialog::CityStreetSearchDialog(DatabaseTask* databaseTask)
 : databaseTask(databaseTask),
   searchStreetAction(new Lum::Model::Action()),
   okAction(new Lum::Model::Action()),
   cityName(new Lum::Model::String(L"")),
   citySearchTimerAction(new Lum::Model::Action()),
   streetName(new Lum::Model::String(L"")),
   streetSearchTimerAction(new Lum::Model::Action()),
   citiesModel(new CitiesModel(cities, new CitiesDataProvider())),
   citySelection(new Lum::Model::SingleLineSelection()),
   streetsModel(new StreetsModel(streets, new StreetsDataProvider())),
   streetSelection(new Lum::Model::SingleLineSelection()),
   hasResult(false)
{
  Observe(searchStreetAction);
  Observe(okAction);
  Observe(GetClosedAction());
  Observe(cityName);
  Observe(citySearchTimerAction);
  Observe(citySelection);
  Observe(streetName);
  Observe(streetSearchTimerAction);
  Observe(streetSelection);

  okAction->Disable();
}

void CityStreetSearchDialog::PreInit()
{
  Lum::Panel            *panel;
  Lum::String           *string;
  Lum::Table            *table;
  Lum::Model::HeaderRef headerModel;
  Lum::Wizard           *wizard;

  wizard=new Lum::Wizard();
  wizard->SetCancelAction(GetClosedAction());

  panel=Lum::VPanel::Create(true,true);

  string=Lum::String::Create(cityName,25,true,false);

  panel->Add(string);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"City",Lum::Base::Size::stdCharWidth,25,true);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,30);
  table->SetMinHeight(Lum::Base::Size::stdCharHeight,5);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(citiesModel);
  table->SetHeaderModel(headerModel);
  table->SetSelection(citySelection);
  table->SetDoubleClickAction(okAction);
  panel->Add(table);

  wizard->AddPage(L"Search city...",panel,searchStreetAction);

  panel=Lum::VPanel::Create(true,true);

  string=Lum::String::Create(streetName,25,true,false);

  panel->Add(string);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"Street",Lum::Base::Size::stdCharWidth,25,true);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,30);
  table->SetMinHeight(Lum::Base::Size::stdCharHeight,5);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(streetsModel);
  table->SetHeaderModel(headerModel);
  table->SetSelection(streetSelection);
  table->SetDoubleClickAction(okAction);
  panel->Add(table);

  wizard->AddPage(L"Search street...",panel,okAction);

  citiesModel->SetEmptyText(L"- no search criteria -");
  streetsModel->SetEmptyText(L"- no search criteria -");

  SetMain(wizard);
}

void CityStreetSearchDialog::FetchCities()
{
  bool limitReached=true;

  citiesModel->Off();

  if (!cityName->Empty()) {
    databaseTask->GetMatchingCities(cityName->Get(),cities,50,limitReached);

    if (limitReached) {
      cities.clear();
      citiesModel->SetEmptyText(L"- too many hits -");
    }
    else if (cities.size()==0) {
      citiesModel->SetEmptyText(L"- no matches -");
    }
    else {
      citiesModel->SetEmptyText(L"");
    }
  }
  else {
    cities.clear();
    citiesModel->SetEmptyText(L"- no search criteria -");
  }

  citiesModel->On();
}

void CityStreetSearchDialog::FetchStreets()
{
  std::cout << "Fetching streets..." << std::endl;
  bool limitReached=true;

  streetsModel->Off();

  if (!streetName->Empty()) {
    std::cout << "Calling database..." << std::endl;

    databaseTask->GetMatchingStreets(resultCity.urbanId,streetName->Get(),streets,50,limitReached);

    if (limitReached) {
      std::cout << "Limit reached." << std::endl;
      streets.clear();
      streetsModel->SetEmptyText(L"- too many hits -");
    }
    else if (streets.size()==0) {
      std::cout << "No matches." << std::endl;
      streetsModel->SetEmptyText(L"- no matches -");
    }
    else {
      std::cout << "Result>0." << std::endl;
      streetsModel->SetEmptyText(L"");
    }
  }
  else {
    std::cout << "No search criteria." << std::endl;
    streets.clear();
    streetsModel->SetEmptyText(L"- no search criteria -");
  }

  streetsModel->On();
}

void CityStreetSearchDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==cityName) {
    if (cityName->Empty()) {
      FetchCities();
    }
    else {
      Lum::OS::display->RemoveTimer(citySearchTimerAction);
      Lum::OS::display->AddTimer(1,150000,citySearchTimerAction);
    }
  }
  else if (model==citySearchTimerAction && citySearchTimerAction->IsFinished()) {
    FetchCities();
  }
  else if (model==citySelection) {
    if (citySelection->HasSelection()) {
      searchStreetAction->Enable();
    }
    else {
      searchStreetAction->Disable();
    }
  }
  else if (model==searchStreetAction && searchStreetAction->IsEnabled() && searchStreetAction->IsFinished()) {
    assert(citySelection->HasSelection());

    resultCity=citiesModel->GetEntry(citySelection->GetLine());
    FetchStreets();
  }
  else if (model==streetName) {
    if (streetName->Empty()) {
      FetchStreets();
    }
    else {
      Lum::OS::display->RemoveTimer(streetSearchTimerAction);
      Lum::OS::display->AddTimer(1,150000,streetSearchTimerAction);
    }
  }
  else if (model==streetSearchTimerAction && streetSearchTimerAction->IsFinished()) {
    FetchStreets();
  }
  else if (model==streetSelection) {
    if (streetSelection->HasSelection()) {
      okAction->Enable();
    }
    else {
      okAction->Disable();
    }
  }
  else if (model==okAction && okAction->IsEnabled() && okAction->IsFinished()) {
    assert(streetSelection->HasSelection());

    resultStreet=streetsModel->GetEntry(streetSelection->GetLine());
    hasResult=true;
    Exit();
  }
  else {
    Dialog::Resync(model,msg);
  }
}

bool CityStreetSearchDialog::HasResult() const
{
  return hasResult;
}

const City& CityStreetSearchDialog::GetResultCity() const
{
  return resultCity;
}


const Street& CityStreetSearchDialog::GetResultStreet() const
{
  return resultStreet;
}


