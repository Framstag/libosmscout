/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2010  Tim Teulings

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

#include "LocationSearchDialog.h"

#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/Database.h>

LocationSearchDialog::LocationSearchDialog(DatabaseTask* databaseTask)
 : databaseTask(databaseTask),
   okAction(new Lum::Model::Action()),
   locationName(new Lum::Model::String(L"")),
   searchTimerAction(new Lum::Model::Action()),
   locationsModel(new LocationsModel(locations)),
   locationSelection(new Lum::Model::SingleLineSelection()),
   hasResult(false)
{
  Observe(okAction);
  Observe(GetClosedAction());
  Observe(locationName);
  Observe(searchTimerAction);
  Observe(locationSelection);

  okAction->Disable();
}

Lum::Object* LocationSearchDialog::GetContent()
{
  Lum::Panel            *panel;
  Lum::String           *string;
  Lum::Table            *table;
  Lum::Model::HeaderRef headerModel;

  panel=Lum::VPanel::Create(true,true);

  string=Lum::String::Create(locationName,25,true,false);

  panel->Add(string);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"Location",Lum::Base::Size::stdCharWidth,25,true);

  table=new Lum::Table();
  table->SetFlex(true,true);
  table->SetMinWidth(Lum::Base::Size::stdCharWidth,60);
  table->SetMinHeight(Lum::Base::Size::stdCharHeight,6);
  table->SetShowHeader(true);
  table->GetTableView()->SetAutoFitColumns(true);
  table->GetTableView()->SetAutoVSize(true);
  table->SetModel(locationsModel);
  table->SetPainter(new LocationsModelPainter());
  table->SetHeaderModel(headerModel);
  table->SetSelection(locationSelection);
  table->SetDoubleClickAction(okAction);
  panel->Add(table);

  locationsModel->SetEmptyText(L"- no search criteria -");

  return panel;
}

void LocationSearchDialog::GetActions(std::vector<Lum::Dlg::ActionInfo>& actions)
{
  Lum::Dlg::ActionDialog::CreateActionInfosOkCancel(actions,okAction,GetClosedAction());
}

void LocationSearchDialog::Search()
{
  bool         limitReached=true;
  size_t       dividerPos;
  std::wstring location(locationName->Get());
  std::wstring city;
  std::wstring street;

  dividerPos=location.find(',');
  if (dividerPos!=std::string::npos) {
    city=location.substr(dividerPos+1);
    street=location.substr(0,dividerPos);
  }
  else {
    city=location;
  }

  while (!street.empty() && street[0]==L' ') {
    street=street.substr(1);
  }

  while (!city.empty() && city[0]==L' ') {
    city=city.substr(1);
  }

  if (!street.empty()) {
    std::cout << "Searching for street '" << Lum::Base::WStringToString(street) << "' in city '" << Lum::Base::WStringToString(city) << "'..." << std::endl;
  }
  else {
    std::cout << "Searching for city '" << Lum::Base::WStringToString(city) << "'..." << std::endl;
  }

  std::list<osmscout::AdminRegion> regions;

  locationsModel->Off();

  locations.clear();

  if (!locationName->Empty()) {
    databaseTask->GetMatchingAdminRegions(city,regions,50,limitReached);

    std::cout << "Result of search for region " << Lum::Base::WStringToString(city) << ": " << regions.size() << std::endl;

    if (limitReached) {
      locationsModel->SetEmptyText(L"- too many hits -");
      locationsModel->On();

      return;
    }
    else if (regions.size()==0) {
      locationsModel->SetEmptyText(L"- no matches -");
      locationsModel->On();

      return;
    }
    else if (street.empty()) {
      locationsModel->SetEmptyText(L"");
    }
  }
  else {
    locationsModel->SetEmptyText(L"- no search criteria -");
    locationsModel->On();
    return;
  }

  if (street.empty()) {
    for (std::list<osmscout::AdminRegion>::const_iterator region=regions.begin();
         region!=regions.end();
         ++region) {
      osmscout::Location location;

      location.name=region->name;
      location.references.push_back(region->reference);
      location.path=region->path;

      locations.push_back(location);
    }

    locationsModel->On();

    return;
  }


  for (std::list<osmscout::AdminRegion>::const_iterator region=regions.begin();
       region!=regions.end();
       ++region) {
    std::list<osmscout::Location> locs;

    databaseTask->GetMatchingLocations(*region,
                                       street,
                                       locs,
                                       50,
                                       limitReached);

    std::cout << "Result of search for street " << Lum::Base::WStringToString(street) << " in region " << region->name << ": " << locs.size() << std::endl;

    if (limitReached) {
      std::cout << "Limit reached." << std::endl;
      locations.clear();
      locationsModel->SetEmptyText(L"- too many hits -");
      locationsModel->On();

      return;
    }

    for (std::list<osmscout::Location>::const_iterator l=locs.begin();
         l!=locs.end();
         ++l) {
      bool found=false;

      for (std::list<osmscout::Location>::const_iterator l2=locations.begin();
           l2!=locations.end();
           ++l2) {
        if (l2->references.front()==l->references.front()) {
          found=true;
          break;
        }
      }

      if (found) {
        continue;
      }

      locations.push_back(*l);

      if (locations.size()>50) {
        std::cout << "Limit reached." << std::endl;
        locations.clear();
        locationsModel->SetEmptyText(L"- too many hits -");
        locationsModel->On();

        return;
      }
    }

  }

  if (locations.size()==0) {
    std::cout << "No matches." << std::endl;
    locationsModel->SetEmptyText(L"- no matches -");
    locationsModel->On();

    return;
  }
  else {
    locationsModel->SetEmptyText(L"");
  }

  locationsModel->On();
}

void LocationSearchDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==searchTimerAction && searchTimerAction->IsFinished()) {
    Search();
  }
  else if (model==locationName) {
    if (locationName->Empty()) {
      Search();
    }
    else {
      Lum::OS::display->RemoveTimer(searchTimerAction);
      Lum::OS::display->AddTimer(1,150000,searchTimerAction);
    }
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

bool LocationSearchDialog::HasResult() const
{
  return hasResult;
}

const osmscout::Location& LocationSearchDialog::GetResult() const
{
  return resultLocation;
}


