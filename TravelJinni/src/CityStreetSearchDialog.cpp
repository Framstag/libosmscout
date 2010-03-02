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
   regionName(new Lum::Model::String(L"")),
   regionSearchTimerAction(new Lum::Model::Action()),
   locationName(new Lum::Model::String(L"")),
   locationSearchTimerAction(new Lum::Model::Action()),
   regionsModel(new RegionsModel(regions,new RegionsDataProvider())),
   regionSelection(new Lum::Model::SingleLineSelection()),
   locationsModel(new LocationsModel(locations,new LocationsDataProvider())),
   locationSelection(new Lum::Model::SingleLineSelection()),
   hasResult(false)
{
  Observe(searchStreetAction);
  Observe(okAction);
  Observe(GetClosedAction());
  Observe(regionName);
  Observe(regionSearchTimerAction);
  Observe(regionSelection);
  Observe(locationName);
  Observe(locationSearchTimerAction);
  Observe(locationSelection);

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

  string=Lum::String::Create(regionName,25,true,false);

  panel->Add(string);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"City",Lum::Base::Size::stdCharWidth,25,true);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,60);
  table->SetMinHeight(Lum::Base::Size::stdCharHeight,6);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(regionsModel);
  table->SetHeaderModel(headerModel);
  table->SetSelection(regionSelection);
  table->SetDoubleClickAction(okAction);
  panel->Add(table);

  wizard->AddPage(L"Search region...",panel,searchStreetAction);

  panel=Lum::VPanel::Create(true,true);

  string=Lum::String::Create(locationName,25,true,false);

  panel->Add(string);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"Street",Lum::Base::Size::stdCharWidth,25,true);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,60);
  table->SetMinHeight(Lum::Base::Size::stdCharHeight,6);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(locationsModel);
  table->SetHeaderModel(headerModel);
  table->SetSelection(locationSelection);
  table->SetDoubleClickAction(okAction);
  panel->Add(table);

  wizard->AddPage(L"Search location...",panel,okAction);

  regionsModel->SetEmptyText(L"- no search criteria -");
  locationsModel->SetEmptyText(L"- no search criteria -");

  SetMain(wizard);
}

void CityStreetSearchDialog::FetchAdminRegions()
{
  bool limitReached=true;

  regionsModel->Off();

  if (!regionName->Empty()) {
    databaseTask->GetMatchingAdminRegions(regionName->Get(),regions,50,limitReached);

    if (limitReached) {
      regions.clear();
      regionsModel->SetEmptyText(L"- too many hits -");
    }
    else if (regions.size()==0) {
      regionsModel->SetEmptyText(L"- no matches -");
    }
    else {
      regionsModel->SetEmptyText(L"");
    }
  }
  else {
    regions.clear();
    regionsModel->SetEmptyText(L"- no search criteria -");
  }

  regionsModel->On();
}

void CityStreetSearchDialog::FetchLocations()
{
  std::cout << "Fetching locations..." << std::endl;
  bool limitReached=true;

  locationsModel->Off();

  if (!locationName->Empty()) {
    std::cout << "Calling database..." << std::endl;

    databaseTask->GetMatchingLocations(resultAdminRegion,
                                       locationName->Get(),
                                       locations,
                                       50,
                                       limitReached);

    if (limitReached) {
      std::cout << "Limit reached." << std::endl;
      locations.clear();
      locationsModel->SetEmptyText(L"- too many hits -");
    }
    else if (locations.size()==0) {
      std::cout << "No matches." << std::endl;
      locationsModel->SetEmptyText(L"- no matches -");
    }
    else {
      std::cout << "Result>0." << std::endl;
      locationsModel->SetEmptyText(L"");
    }
  }
  else {
    std::cout << "No search criteria." << std::endl;
    locations.clear();
    locationsModel->SetEmptyText(L"- no search criteria -");
  }

  locationsModel->On();
}

void CityStreetSearchDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==regionName) {
    if (regionName->Empty()) {
      FetchAdminRegions();
    }
    else {
      Lum::OS::display->RemoveTimer(regionSearchTimerAction);
      Lum::OS::display->AddTimer(1,150000,regionSearchTimerAction);
    }
  }
  else if (model==regionSearchTimerAction && regionSearchTimerAction->IsFinished()) {
    FetchAdminRegions();
  }
  else if (model==regionSelection) {
    if (regionSelection->HasSelection()) {
      searchStreetAction->Enable();
    }
    else {
      searchStreetAction->Disable();
    }
  }
  else if (model==searchStreetAction && searchStreetAction->IsEnabled() && searchStreetAction->IsFinished()) {
    assert(regionSelection->HasSelection());

    resultAdminRegion=regionsModel->GetEntry(regionSelection->GetLine());
    FetchLocations();
  }
  else if (model==locationName) {
    if (locationName->Empty()) {
      FetchLocations();
    }
    else {
      Lum::OS::display->RemoveTimer(locationSearchTimerAction);
      Lum::OS::display->AddTimer(1,150000,locationSearchTimerAction);
    }
  }
  else if (model==locationSearchTimerAction && locationSearchTimerAction->IsFinished()) {
    FetchLocations();
  }
  else if (model==locationSelection) {
    if (locationSelection->HasSelection()) {
      okAction->Enable();
    }
    else {
      okAction->Disable();
    }
  }
  else if (model==okAction && okAction->IsEnabled() && okAction->IsFinished()) {
    assert(locationSelection->HasSelection());

    resultLocation=locationsModel->GetEntry(locationSelection->GetLine());
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

const osmscout::AdminRegion& CityStreetSearchDialog::GetResultAdminRegion() const
{
  return resultAdminRegion;
}


const osmscout::Location& CityStreetSearchDialog::GetResultLocation() const
{
  return resultLocation;
}


