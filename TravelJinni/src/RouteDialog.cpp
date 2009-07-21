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

#include "RouteDialog.h"

#include <Lum/Label.h>
#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/Database.h>

#include "CityStreetSearchDialog.h"

RouteDialog::RouteDialog(DatabaseTask* databaseTask)
 : databaseTask(databaseTask),
   okAction(new Lum::Model::Action()),
   start(new Lum::Model::String()),
   hasStart(false),
   startAction(new Lum::Model::Action()),
   end(new Lum::Model::String()),
   hasEnd(false),
   endAction(new Lum::Model::Action()),
   hasResult(false)
{
  Observe(okAction);
  Observe(GetClosedAction());
  Observe(startAction);
  Observe(endAction);

  start->Disable();
  end->Disable();
  okAction->Disable();
}

Lum::Object* RouteDialog::GetContent()
{
  Lum::Label            *label;
  Lum::Panel            *panel,*sub;
  Lum::Model::HeaderRef headerModel;

  panel=Lum::VPanel::Create(true,true);

  label=Lum::Label::Create(true,false);

  sub=Lum::HPanel::Create(true,false);
  sub->Add(Lum::String::Create(start,30,true,false));
  sub->Add(Lum::Button::Create(L"...",startAction));
  label->AddLabel(L"Start:",sub);

  sub=Lum::HPanel::Create(true,false);
  sub->Add(Lum::String::Create(end,30,true,false));
  sub->Add(Lum::Button::Create(L"...",endAction));
  label->AddLabel(L"End:",sub);

  panel->Add(label);

  return panel;
}

void RouteDialog::GetActions(std::vector<Lum::Dlg::ActionInfo>& actions)
{
  Lum::Dlg::ActionDialog::CreateActionInfosOkCancel(actions,okAction,GetClosedAction());
}

void RouteDialog::Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
{
  if (model==GetClosedAction() &&  GetClosedAction()->IsFinished()) {
    Exit();
  }
  else if (model==startAction && startAction->IsEnabled() && startAction->IsFinished()) {
    bool hasResult=false;

    CityStreetSearchDialog *dialog;

    dialog=new CityStreetSearchDialog(databaseTask);
    dialog->SetParent(this);
    if (dialog->Open()) {
      dialog->EventLoop();
      dialog->Close();

      hasResult=dialog->HasResult();
      if (dialog->HasResult()) {
        startCity=dialog->GetResultCity();
        startStreet=dialog->GetResultStreet();
      }
    }

    delete dialog;

    if (hasResult) {
      hasStart=true;

      start->Set(Lum::Base::UTF8ToWString(startCity.name)+
                 L", "+
                 Lum::Base::UTF8ToWString(startStreet.name));

      if (hasStart && hasEnd) {
        okAction->Enable();
      }
    }
  }
  else if (model==endAction && endAction->IsEnabled() && endAction->IsFinished()) {
    bool hasResult=false;

    CityStreetSearchDialog *dialog;

    dialog=new CityStreetSearchDialog(databaseTask);
    dialog->SetParent(this);
    if (dialog->Open()) {
      dialog->EventLoop();
      dialog->Close();

      hasResult=dialog->HasResult();
      if (dialog->HasResult()) {
        endCity=dialog->GetResultCity();
        endStreet=dialog->GetResultStreet();
      }
    }

    delete dialog;

    if (hasResult) {
      hasEnd=true;

      end->Set(Lum::Base::UTF8ToWString(endCity.name)+
               L", "+
               Lum::Base::UTF8ToWString(endStreet.name));

      if (hasStart && hasEnd) {
        okAction->Enable();
      }
    }
  }
  else if (model==okAction && okAction->IsEnabled() && okAction->IsFinished()) {
    assert(hasStart && hasEnd);

    std::cout << "Route: " << startStreet.reference.id << " => " << endStreet.reference.id << std::endl;

    /*
    result=citiesModel->GetEntry(citySelection->GetLine());
    hasResult=true;*/
    Exit();
  }
  else {
    Dialog::Resync(model,msg);
  }
}

bool RouteDialog::HasResult() const
{
  return hasResult;
}

