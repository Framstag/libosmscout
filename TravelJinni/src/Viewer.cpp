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

#include <cmath>
#include <iostream>
#include <iomanip>

#include <Lum/Base/DateTime.h>
#include <Lum/Base/L10N.h>
#include <Lum/Base/Path.h>
#include <Lum/Base/String.h>

#include <Lum/Def/AppInfo.h>
#include <Lum/Def/Menu.h>

#include <Lum/Dlg/About.h>

#include <Lum/Model/String.h>
#include <Lum/Model/Table.h>

#include <Lum/OS/Cairo/Bitmap.h>
#include <Lum/OS/Cairo/Display.h>
#include <Lum/OS/Cairo/DrawInfo.h>

#include <Lum/OS/Bitmap.h>
#include <Lum/OS/Display.h>
#include <Lum/OS/Driver.h>
#include <Lum/OS/Main.h>
#include <Lum/OS/Thread.h>

#include <Lum/Dialog.h>
#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/StyleConfigLoader.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeConfigLoader.h>

#include "MapPainter.h"

#include "CitySearchDialog.h"
#include "CityStreetSearchDialog.h"
#include "RouteDialog.h"
#include "DatabaseTask.h"

static Lum::Def::AppInfo info;

static TypeConfig            typeConfig;
static StyleConfig           styleConfig;
static Database              *database=NULL;
static Lum::Model::ActionRef jobFinishedAction;
static DatabaseTask          *databaseTask=NULL;

class Map : public Lum::Object
{
private:
  double                lon;
  double                lat;
  double                magnification;
  double                startLon,startLat;
  int                   startX,startY;
  bool                  requestNewMap;

public:
  Map()
  : lon(7.13601),
    lat(50.68924),
    magnification(/*2*2*2**/2*1024),
    requestNewMap(true)
  {
    SetCanFocus(true);
    RequestFocus();

    Observe(jobFinishedAction);

  }

  ~Map()
  {
    // no code
  }

  void Layout()
  {
    Object::Layout();
  }

  void CalcSize()
  {
    width=800;
    height=480;

    minWidth=width;
    minHeight=height;

    maxWidth=width;
    maxHeight=height;

    Object::CalcSize();
  }

  void Draw(int x, int y, size_t w, size_t h)
  {
    Object::Draw(x,y,w,h);

    if (!OIntersect(x,y,w,h)) {
      return;
    }

    /* --- */

    Lum::OS::DrawInfo *draw=GetDrawInfo();

    if (!databaseTask->DrawResult(draw,
                                  this->x,this->y,
                                  this->width,this->height,
                                  lon,lat,
                                  magnification) && requestNewMap) {
      RequestNewMap();
    }


  }

  void RequestNewMap()
  {
    Job *job=new Job();

    std::cout << "Requesting " << lon << ", " << lat << " with magnification " << magnification << "x" << std::endl;

    job->lon=lon;
    job->lat=lat;
    job->magnification=magnification;
    job->width=width;
    job->height=height;

    databaseTask->PostJob(job);
  }

  void HandleMouseMove(const Lum::OS::MouseEvent& event)
  {
    double lonMin,latMin,lonMax,latMax;

    MapPainter::GetDimensions(lon,lat,
                              magnification,
                              width,height,
                              lonMin,latMin,lonMax,latMax);

    lon=startLon-(lonMax-lonMin)*(event.x-startX)/width;
    lat=startLat+(latMax-latMin)*(event.y-startY)/height;
  }

  void ZoomIn(double zoomFactor)
  {
    if (magnification*zoomFactor>100000) {
      magnification=100000;
    }
    else {
      magnification*=zoomFactor;
    }

    RequestNewMap();
  }

  void ZoomOut(double zoomFactor)
  {
    if (magnification/zoomFactor<1) {
      magnification=1;
    }
    else {
      magnification/=zoomFactor;
    }

    RequestNewMap();
  }

  bool HandleMouseEvent(const Lum::OS::MouseEvent& event)
  {
    if (!visible) {
      return false;
    }

    if (event.type==Lum::OS::MouseEvent::down &&
        PointIsIn(event) &&
        event.button==Lum::OS::MouseEvent::button1) {
      startLon=lon;
      startLat=lat;
      startX=event.x;
      startY=event.y;
      return true;
    }
    else if (event.IsGrabEnd() && event.button==Lum::OS::MouseEvent::button1) {
      HandleMouseMove(event);
      Redraw();
    }
    else if (event.type==Lum::OS::MouseEvent::move &&
             event.IsGrabed() &&
             event.button==Lum::OS::MouseEvent::none &&
             event.qualifier==Lum::OS::qualifierButton1) {
      HandleMouseMove(event);
      requestNewMap=false;
      Redraw();
      requestNewMap=true;
      return true;
    }
    else if (event.type==Lum::OS::MouseEvent::down &&
             event.button==Lum::OS::MouseEvent::button4) {
      ZoomIn(1.2);

      return true;
    }
    else if (event.type==Lum::OS::MouseEvent::down &&
             event.button==Lum::OS::MouseEvent::button5) {
      ZoomOut(1.2);

      return true;
    }

    return false;
  }

  bool HandleKeyEvent(const Lum::OS::KeyEvent& event)
  {
    if (event.type==Lum::OS::KeyEvent::down) {
      if (event.key==Lum::OS::keyLeft) {
        double lonMin,latMin,lonMax,latMax;

        MapPainter::GetDimensions(lon,lat,
                                  magnification,
                                  width,height,
                                  lonMin,latMin,lonMax,latMax);

        lon-=(lonMax-lonMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.key==Lum::OS::keyRight) {
        double lonMin,latMin,lonMax,latMax;

        MapPainter::GetDimensions(lon,lat,
                                  magnification,
                                  width,height,
                                  lonMin,latMin,lonMax,latMax);

        lon+=(lonMax-lonMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.key==Lum::OS::keyUp) {
        double lonMin,latMin,lonMax,latMax;

        MapPainter::GetDimensions(lon,lat,
                                  magnification,
                                  width,height,
                                  lonMin,latMin,lonMax,latMax);

        lat+=(latMax-latMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.key==Lum::OS::keyDown) {
        double lonMin,latMin,lonMax,latMax;

        MapPainter::GetDimensions(lon,lat,
                                  magnification,
                                  width,height,
                                  lonMin,latMin,lonMax,latMax);

        lat-=(latMax-latMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.text==L"+") {
        ZoomIn(2.0);

        return true;
      }
      else if (event.text==L"-") {
        ZoomOut(2.0);

        RequestNewMap();

        return true;
      }
    }

    return false;
  }

  void Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
  {
    if (model==jobFinishedAction && jobFinishedAction->IsFinished()) {
      Redraw();
    }

    Object::Resync(model,msg);
  }

  void ShowReference(const Reference& reference, const Mag& magnification)
  {
    if (reference.GetType()==refNode) {
      Node node;

      if (database->GetNode(reference.GetId(),node)) {
        lon=node.lon;
        lat=node.lat;
        this->magnification=magnification;

        RequestNewMap();
      }
    }
    else if (reference.GetType()==refArea) {
      std::cout << "Showing area " << reference.GetId() << std::endl;
      assert(false);
    }
    else if (reference.GetType()==refWay) {
      Way way;

      if (database->GetWay(reference.GetId(),way)) {
        lon=way.nodes[0].lon;
        lat=way.nodes[0].lat;
        this->magnification=magnification;

        RequestNewMap();
      }
    }
    else {
      assert(false);
    }
  }
};

class MainDialog : public Lum::Dialog
{
private:
  Lum::Model::ActionRef       searchCityAction;
  Lum::Model::ActionRef       searchAddressAction;
  Lum::Model::ActionRef       routeAction;
  Lum::Model::ActionRef       debugStatisticsAction;
  Lum::Model::ActionRef       aboutAction;
  Map                         *map;
  RouteDialog::RouteSelection routeSelection;

public:
  MainDialog()
   : searchCityAction(new Lum::Model::Action()),
     searchAddressAction(new Lum::Model::Action()),
     routeAction(new Lum::Model::Action()),
     debugStatisticsAction(new Lum::Model::Action()),
     aboutAction(new Lum::Model::Action())
  {
    Observe(GetClosedAction());
    Observe(searchCityAction);
    Observe(searchAddressAction);
    Observe(routeAction);
    Observe(debugStatisticsAction);
    Observe(aboutAction);
  }

  void PreInit()
  {
    map=new Map();
    map->SetFlex(true,true);

    SetMain(map);

    Lum::Def::Menu *menu=Lum::Def::Menu::Create();

    menu
      ->GroupProject()
        ->ActionQuit(GetClosedAction())
      ->End()
      ->Group(L"_Search")
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"Search _city")
                                  .SetShortcut(Lum::OS::qualifierControl,L"c"),
                                  searchCityAction))
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"Search _address")
                                  .SetShortcut(Lum::OS::qualifierControl,L"a"),
                                  searchAddressAction))
        ->Separator()
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"_Route")
                                  .SetShortcut(Lum::OS::qualifierControl,L"r"),
                                  routeAction))
      ->End()
      ->Group(L"Debug")
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"Dump _statistics")
                                  .SetShortcut(Lum::OS::qualifierControl,L"s"),
                                  debugStatisticsAction))
      ->End()
      ->GroupHelp()
        //->ActionHelp()(
        ->ActionAbout(aboutAction)
      ->End();

    SetMenu(menu);

    Dialog::PreInit();
  }

  void Resync(Lum::Base::Model* model, const Lum::Base::ResyncMsg& msg)
  {
    if (model==GetClosedAction() && GetClosedAction()->IsFinished()) {
      std::cout << "Quitting..." << std::endl;
    }
    else if (model==searchCityAction && searchCityAction->IsFinished()) {
      City city;
      bool hasResult=false;

      CitySearchDialog *dialog;

      dialog=new CitySearchDialog(databaseTask);
      dialog->SetParent(this);
      if (dialog->Open()) {
        dialog->EventLoop();
        dialog->Close();

        hasResult=dialog->HasResult();
        if (dialog->HasResult()) {
          city=dialog->GetResult();
        }
      }

      delete dialog;

      if (hasResult) {
        map->ShowReference(city.reference,magCity);
      }
    }
    else if (model==searchAddressAction && searchAddressAction->IsFinished()) {
      Street street;
      bool hasResult=false;

      CityStreetSearchDialog *dialog;

      dialog=new CityStreetSearchDialog(databaseTask);
      dialog->SetParent(this);
      if (dialog->Open()) {
        dialog->EventLoop();
        dialog->Close();

        hasResult=dialog->HasResult();
        if (dialog->HasResult()) {
          street=dialog->GetResultStreet();
        }
      }

      delete dialog;

      if (hasResult) {
        map->ShowReference(street.reference,magVeryClose);
      }
    }
    else if (model==routeAction && routeAction->IsFinished()) {
      RouteDialog *dialog;

      dialog=new RouteDialog(databaseTask,routeSelection);
      dialog->SetParent(this);
      if (dialog->Open()) {
        dialog->EventLoop();
        dialog->Close();

        routeSelection=dialog->GetResult();
      }

      delete dialog;
    }
    else if (model==debugStatisticsAction && debugStatisticsAction->IsFinished()) {
      database->DumpStatistics();
    }
    else if (model==aboutAction && aboutAction->IsFinished()) {
      Lum::Dlg::About::Show(this,info);
    }

    Dialog::Resync(model,msg);
  }
};


class Main : public Lum::OS::MainDialog<MainDialog>
{
public:
  bool Prepare()
  {
    database=new Database();
    jobFinishedAction=new Lum::Model::Action();

#if defined(APP_DATADIR)
    Lum::Base::Path::SetApplicationDataDir(Lum::Base::StringToWString(APP_DATADIR));
#endif

    info.SetProgram(Lum::Base::StringToWString(PACKAGE_NAME));
    info.SetVersion(Lum::Base::StringToWString(PACKAGE_VERSION));
    info.SetDescription(_(L"ABOUT_DESC",L"A simple OSM map viewer..."));
    info.SetAuthor(L"Tim Teulings");
    info.SetContact(L"Tim Teulings <tim@teulings.org>");
    info.SetCopyright(L"(c) 2009, Tim Teulings");
    info.SetLicense(L"GNU Public License");

    if (!Lum::OS::MainDialog<MainDialog>::Prepare()) {
      return false;
    }

    if (!LoadTypeConfig("map.ost.xml",typeConfig)) {
      std::cerr << "Cannot load type configuration!" << std::endl;
    }

    if (!LoadStyleConfig("standard.oss.xml",typeConfig,styleConfig)) {
      std::cerr << "Cannot load style configuration!" << std::endl;
    }

    Lum::Base::Path path;

    if (Lum::OS::display->GetArgCount()==1) {
      path.SetNativeDir(Lum::OS::display->GetArg(0));
    }
    else {
      path.SetNativeDir(Lum::Base::Path::GetCWD());
    }

    std::string     pathString=Lum::Base::WStringToString(path.GetPath());

    if (!database->Open(pathString)) {
      std::cerr << "Cannot initialize database!" << std::endl;
    }

    database->DumpStatistics();

    databaseTask=new DatabaseTask(database,
                                  jobFinishedAction);

    databaseTask->SetStyle(&styleConfig);

    databaseTask->Start();

    /*
    Way              way;
    RouteData        routeData;
    RouteDescription routeDescription;
    database->CalculateRoute(typeConfig,
                             14331559,138190834,
                             10414977,254429626,
                             routeData);
    database->DumpStatistics();

    database->TransformRouteDataToRouteDescription(routeData,routeDescription);

    for (std::list<RouteDescription::RouteStep>::const_iterator step=routeDescription.Steps().begin();
         step!=routeDescription.Steps().end();
         ++step) {
      std::cout << std::fixed << std::setprecision(1);
      std::cout << step->GetDistance() << "km ";

      switch (step->GetAction()) {
      case RouteDescription::start:
        std::cout << "Start at ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case RouteDescription::drive:
        std::cout << "drive along ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case RouteDescription::switchRoad:
        std::cout << "turn into ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      case RouteDescription::reachTarget:
        std::cout << "Arriving at ";
        if (!step->GetName().empty()) {
          std::cout << step->GetName();

          if (!step->GetRefName().empty()) {
            std::cout << " (" << step->GetRefName() << ")";
          }
        }
        else {
          std::cout << step->GetRefName();
        }
        break;
      }

      std::cout << std::endl;
    }

    database->TransformRouteDataToWay(routeData,way);
    databaseTask->AddRoute(way);*/

    return true;
  }

  void Cleanup()
  {
    if (databaseTask!=NULL) {
      databaseTask->Finish();
      databaseTask->PostJob(NULL);
      databaseTask->Join();
      delete databaseTask;
      databaseTask=NULL;
    }

    Lum::OS::MainDialog<MainDialog>::Cleanup();

    if (database->IsOpen()) {
      database->Close();
    }
    delete database;
    database=NULL;
  }
};


LUM_MAIN(Main,L"Viewer")

