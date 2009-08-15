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

#include "CitySearchDialog.h"

#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/Database.h>

CitySearchDialog::CitySearchDialog(DatabaseTask* databaseTask)
 : databaseTask(databaseTask),
   okAction(new Lum::Model::Action()),
   cityName(new Lum::Model::String(L"")),
   searchTimerAction(new Lum::Model::Action()),
   citiesModel(new CitiesModel(cities, new CitiesDataProvider())),
   citySelection(new Lum::Model::SingleLineSelection()),
   hasResult(false)
{
  Observe(okAction);
  Observe(GetClosedAction());
  Observe(cityName);
  Observe(searchTimerAction);
  Observe(citySelection);

  okAction->Disable();
}

Lum::Object* CitySearchDialog::GetContent()
{
  Lum::Panel            *panel;
  Lum::String           *string;
  Lum::Table            *table;
  Lum::Model::HeaderRef headerModel;

  panel=Lum::VPanel::Create(true,true);

  string=Lum::String::Create(cityName,25,true,false);

  panel->Add(string);

  panel->AddSpace();

  headerModel=new Lum::Model::HeaderImpl();
  headerModel->AddColumn(L"Name",Lum::Base::Size::stdCharWidth,25,true);

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

  citiesModel->SetEmptyText(L"- no search criteria -");

  return panel;
}

void CitySearchDialog::GetActions(std::vector<Lum::Dlg::ActionInfo>& actions)
{
  Lum::Dlg::ActionDialog::CreateActionInfosOkCancel(actions,okAction,GetClosedAction());
}

void CitySearchDialog::FetchCities()
{
  bool limitReached=true;

  citiesModel->Off();

  if (!cityName->Empty()) {
    databaseTask->GetMatchingCities(cityName->Get(),cities,50,limitReached);

    if (limitReached) {
      cities.clear();
      citiesModel->SetEmptyText(L"- too many hits -");
    }
    else if (cities.size()>0) {
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

void CitySearchDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==searchTimerAction && searchTimerAction->IsFinished()) {
    FetchCities();
  }
  else if (model==cityName) {
    if (cityName->Empty()) {
      FetchCities();
    }
    else {
      Lum::OS::display->RemoveTimer(searchTimerAction);
      Lum::OS::display->AddTimer(1,150000,searchTimerAction);
    }
  }
  else if (model==citySelection) {
    if (citySelection->HasSelection()) {
      okAction->Enable();
    }
    else {
      okAction->Disable();
    }
  }
  else if (model==okAction && okAction->IsEnabled() && okAction->IsFinished()) {
    assert(citySelection->HasSelection());

    result=citiesModel->GetEntry(citySelection->GetLine());
    hasResult=true;
    Exit();
  }
  else {
    Dialog::Resync(model,msg);
  }
}

bool CitySearchDialog::HasResult() const
{
  return hasResult;
}

const City& CitySearchDialog::GetResult() const
{
  return result;
}


