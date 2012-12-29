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
#include <Lum/Def/Props.h>

#include <Lum/Dlg/About.h>
#include <Lum/Dlg/Msg.h>
#include <Lum/Dlg/Properties.h>

#include <Lum/Manager/FileSystem.h>

#include <Lum/Model/String.h>
#include <Lum/Model/Table.h>

#include <Lum/OS/Bitmap.h>
#include <Lum/OS/Display.h>
#include <Lum/OS/Driver.h>
#include <Lum/OS/Thread.h>

#include <Lum/Features.h>

#include <Lum/Application.h>
#include <Lum/Dialog.h>
#include <Lum/Object.h>
#include <Lum/Panel.h>
#include <Lum/String.h>
#include <Lum/Table.h>

#include <osmscout/util/Projection.h>

#include "config.h"

#include "Configuration.h"
#include "LocationSearchDialog.h"
#include "RouteDialog.h"
#include "DatabaseTask.h"

static Lum::Def::AppInfo info;

static osmscout::StyleConfig *styleConfig;
static osmscout::Database    *database=NULL;
static osmscout::Router      *router=NULL;
static Lum::Model::ActionRef jobFinishedAction;
static DatabaseTask          *databaseTask=NULL;

class MapControl : public Lum::Object
{
private:
  bool                    initial;
  double                  lon;
  double                  lat;
  osmscout::Magnification magnification;
  double                  startLon,startLat;
  int                     startX,startY;
  bool                    requestNewMap;
  Lum::OS::ColorRef       backgroundColor;

public:
  MapControl()
  : initial(true),
    lon(7.13601),
    lat(50.68924),
    magnification(2*1024),
    requestNewMap(true),
    backgroundColor(241.0/255,238.0/255,233.0/255,1.0)
  {
    SetCanFocus(true);
    RequestFocus();

    Observe(jobFinishedAction);
  }

  virtual ~MapControl()
  {
    // no code
  }

  void CalcSize()
  {
#if defined(LUM_HAVE_LIB_HILDON)
    width=200;
    height=200;
#else
    width=800;
    height=480;
#endif

    minWidth=width;
    minHeight=height;

    Object::CalcSize();
  }

  void Draw(Lum::OS::DrawInfo* draw, int x, int y, size_t w, size_t h)
  {
    Object::Draw(draw,x,y,w,h);

    if (!OIntersect(x,y,w,h)) {
      return;
    }

    /* --- */

    draw->PushForeground(backgroundColor);
    draw->FillRectangle(this->x,this->y,this->width,this->height);
    draw->PopForeground();

    osmscout::MercatorProjection projection;

    bool drawResultFinal=databaseTask->DrawResult(GetWindow(),
                                                  draw,
                                                  this->x,this->y,
                                                  this->width,this->height,
                                                  lon,lat,
                                                  magnification,
                                                  projection);

    double lonMin,lonMax,latMin,latMax;

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    double d=(lonMax-lonMin)*2*M_PI/360;
    double scaleSize;
    size_t minScaleWidth=width/20;
    size_t maxScaleWidth=width/10;
    double scaleValue=d*180*60/M_PI*1852.216/(width/minScaleWidth);

    //std::cout << "1/10 screen (" << width/10 << " pixels) are: " << scaleValue << " meters" << std::endl;

    scaleValue=pow(10,floor(log10(scaleValue))+1);
    scaleSize=scaleValue/(d*180*60/M_PI*1852.216/width);

    if (scaleSize>minScaleWidth && scaleSize/2>minScaleWidth && scaleSize/2<=maxScaleWidth) {
      scaleValue=scaleValue/2;
      scaleSize=scaleSize/2;
    }
    else if (scaleSize>minScaleWidth && scaleSize/5>minScaleWidth && scaleSize/5<=maxScaleWidth) {
      scaleValue=scaleValue/5;
      scaleSize=scaleSize/5;
    }
    else if (scaleSize>minScaleWidth && scaleSize/10>minScaleWidth && scaleSize/10<=maxScaleWidth) {
      scaleValue=scaleValue/10;
      scaleSize=scaleSize/10;
    }

    //std::cout << "VisualScale: value: " << scaleValue << " pixel: " << scaleSize << std::endl;

    std::wstring unit=L"m";

    if (scaleValue>=1000) {
      scaleValue=scaleValue/1000;
      unit=L"km";
    }

    draw->PushForeground(Lum::OS::Display::blackColor);

    draw->PushPen(2,Lum::OS::DrawInfo::penNormal);

    draw->DrawLine(this->x+width/20,this->y+height*19/20,
                   this->x+width/20+scaleSize-1,this->y+height*19/20);
    draw->DrawLine(this->x+width/20,this->y+height*19/20,
                   this->x+width/20,this->y+height*19/20-height/40);
    draw->DrawLine(this->x+width/20+scaleSize-1,this->y+height*19/20,
                   this->x+width/20+scaleSize-1,this->y+height*19/20-height/40);

    draw->PushFont(Lum::OS::display->GetFont());
    draw->DrawString(this->x+width/20+scaleSize-1+10,this->y+height*19/20,
                     Lum::Base::NumberToWString((size_t)scaleValue)+unit);
    draw->PopFont();

    draw->PopForeground();

    if (!drawResultFinal && requestNewMap) {
      RequestNewMap();
    }
  }

  void RequestNewMap()
  {
    Job *job=new Job();

    if (initial) {
      double minLat,minLon,maxLat,maxLon;

      if (databaseTask->GetBoundingBox(minLat,minLon,maxLat,maxLon)) {
        size_t zoom=1;
        double dlat=360;
        double dlon=180;

        lat=minLat+(maxLat-minLat)/2;
        lon=minLon+(maxLon-minLon)/2;

        while (dlat>maxLat-minLat &&
               dlon>maxLon-minLon) {
          zoom=zoom*2;
          dlat=dlat/2;
          dlon=dlon/2;
        }

        magnification=zoom;

        std::cout << "Showing initial bounding box [";
        std::cout << minLat <<"," << minLon << " - " << maxLat << "," << maxLon << "]";
        std::cout << ", mag. " << magnification.GetMagnification() << "x" << std::endl;

        initial=false;
      }
    }

    job->lon=lon;
    job->lat=lat;
    job->magnification=magnification;
    job->width=width;
    job->height=height;

    databaseTask->PostJob(job);
  }

  void HandleMouseMove(const Lum::OS::MouseEvent& event)
  {
    double                       olon, olat;
    double                       tlon, tlat;
    osmscout::MercatorProjection projection;

    projection.Set(lon,lat,
                   magnification,
                   width,height);

    // Get origin coordinates
    projection.PixelToGeo(0,0,
                          olon,olat);

    // Get current mouse pos coordinates (relative to drag start)
    projection.PixelToGeo(event.x-startX,
                          event.y-startY,
                          tlon,tlat);

    lon=startLon-(tlon-olon);
    lat=startLat-(tlat-olat);
  }

  void ZoomIn(double zoomFactor)
  {
    if (magnification.GetMagnification()*zoomFactor>200000) {
      magnification.SetMagnification(200000);
    }
    else {
      magnification.SetMagnification(magnification.GetMagnification()*zoomFactor);
    }

    RequestNewMap();
  }

  void ZoomOut(double zoomFactor)
  {
    if (magnification.GetMagnification()/zoomFactor<1) {
      magnification.SetMagnification(1);
    }
    else {
      magnification.SetMagnification(magnification.GetMagnification()/zoomFactor);
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
      requestNewMap=false;
      startLon=lon;
      startLat=lat;
      startX=event.x;
      startY=event.y;
      return true;
    }
    else if (event.IsGrabEnd() &&
             event.button==Lum::OS::MouseEvent::button1) {
      HandleMouseMove(event);
      requestNewMap=true;
      Redraw();
    }
    else if (event.type==Lum::OS::MouseEvent::move &&
             event.IsGrabed() &&
             event.button==Lum::OS::MouseEvent::none &&
             event.qualifier==Lum::OS::qualifierButton1) {
      HandleMouseMove(event);
      Redraw();
      return true;
    }
    else if (event.type==Lum::OS::MouseEvent::down &&
             event.button==Lum::OS::MouseEvent::button4) {
      ZoomIn(1.35);

      return true;
    }
    else if (event.type==Lum::OS::MouseEvent::down &&
             event.button==Lum::OS::MouseEvent::button5) {
      ZoomOut(1.35);

      return true;
    }

    return false;
  }

  bool HandleKeyEvent(const Lum::OS::KeyEvent& event)
  {
    if (event.type==Lum::OS::KeyEvent::down) {
      if (event.key==Lum::OS::keyLeft) {
        osmscout::MercatorProjection projection;
        double                       lonMin,latMin,lonMax,latMax;

        projection.Set(lon,lat,
                       magnification,
                       width,height);


        projection.GetDimensions(lonMin,latMin,lonMax,latMax);

        lon-=(lonMax-lonMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.key==Lum::OS::keyRight) {
        osmscout::MercatorProjection projection;
        double                       lonMin,latMin,lonMax,latMax;

        projection.Set(lon,lat,
                       magnification,
                       width,height);

        projection.GetDimensions(lonMin,latMin,lonMax,latMax);

        lon+=(lonMax-lonMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.key==Lum::OS::keyUp) {
        osmscout::MercatorProjection projection;
        double                       lonMin,latMin,lonMax,latMax;

        projection.Set(lon,lat,
                       magnification,
                       width,height);

        projection.GetDimensions(lonMin,latMin,lonMax,latMax);

        lat+=(latMax-latMin)*0.3;

        RequestNewMap();

        return true;
      }
      else if (event.key==Lum::OS::keyDown) {
        osmscout::MercatorProjection projection;
        double                       lonMin,latMin,lonMax,latMax;

        projection.Set(lon,lat,
                       magnification,
                       width,height);

        projection.GetDimensions(lonMin,latMin,lonMax,latMax);

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

  void ShowReference(const osmscout::ObjectFileRef& reference,
                     const osmscout::Magnification& magnification)
  {
    std::cout << "Showing " << reference.GetTypeName() << " at file offset "<< reference.GetFileOffset() << "..." << std::endl;

    if (reference.GetType()==osmscout::refNode) {
      osmscout::NodeRef node;

      if (database->GetNodeByOffset(reference.GetFileOffset(),node)) {
        lon=node->GetLon();
        lat=node->GetLat();
        this->magnification=magnification;

        RequestNewMap();
      }
    }
    else if (reference.GetType()==osmscout::refWay) {
      osmscout::WayRef way;

      if (database->GetWayByOffset(reference.GetFileOffset(),way)) {
        if (way->GetCenter(lat,lon)) {
          this->magnification=magnification;

          RequestNewMap();
        }
      }
    }
    else if (reference.GetType()==osmscout::refRelation) {
      osmscout::RelationRef relation;

      if (database->GetRelationByOffset(reference.GetFileOffset(),relation)) {
        if (relation->GetCenter(lat,lon)) {
          this->magnification=magnification;

          RequestNewMap();
        }
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
  Lum::Model::ActionRef       locationSearchAction;
  Lum::Model::ActionRef       routeAction;
  Lum::Model::ActionRef       settingsAction;
  Lum::Model::ActionRef       debugFlushCacheAction;
  Lum::Model::ActionRef       debugStatisticsAction;
  Lum::Model::ActionRef       aboutAction;
  MapControl                  *map;

public:
  MainDialog()
   : locationSearchAction(new Lum::Model::Action()),
     routeAction(new Lum::Model::Action()),
     settingsAction(new Lum::Model::Action()),
     debugFlushCacheAction(new Lum::Model::Action()),
     debugStatisticsAction(new Lum::Model::Action()),
     aboutAction(new Lum::Model::Action()),
     map(NULL)
  {
    GetWindow()->SetScreenOrientationHint(Lum::OS::Window::screenOrientationBothSupported);

    Observe(GetOpenedAction());
    Observe(GetClosedAction());
    Observe(locationSearchAction);
    Observe(routeAction);
    Observe(settingsAction);
    Observe(debugFlushCacheAction);
    Observe(debugStatisticsAction);
    Observe(aboutAction);
  }

  void PreInit()
  {
    map=new MapControl();
    map->SetFlex(true,true);

    SetMain(map);

    Lum::Def::Menu *menu=Lum::Def::Menu::Create();

    menu
      ->GroupProject()
        ->ActionQuit(GetClosedAction())
      ->End()
      ->Group(L"_Search")
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"Search _location")
                                  .SetShortcut(Lum::OS::qualifierControl,L"f"),
                                  locationSearchAction))
        ->Separator()
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"_Route")
                                  .SetShortcut(Lum::OS::qualifierControl,L"r"),
                                  routeAction))
      ->End()
      ->GroupEdit()
        ->ActionSettings(settingsAction)
      ->End()
      ->Group(L"Debug")
        ->Action(Lum::Def::Action(Lum::Def::Desc(L"_Flush Cache"),
                                  debugFlushCacheAction))
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
    if (model==GetOpenedAction() && GetOpenedAction()->IsFinished()) {
      if (!LoadConfig()) {
        Lum::Dlg::Msg::ShowOk(this,
                              L"Cannot load configuration!",
                              L"Cannot load configuration!");
      }

      if (!currentMap.empty() && !currentStyle.empty()) {
        if (databaseTask->Open(currentMap)) {
          if (databaseTask->LoadStyleConfig(currentStyle,styleConfig)) {
            databaseTask->SetStyle(styleConfig);
          }
          else {
            std::cerr << "Cannot load style configuration '" << Lum::Base::WStringToString(currentStyle) << "'" << std::endl;
          }
        }
        else {
          std::cerr << "Cannot initialize database!" << std::endl;
        }
      }
      else {
        Lum::Dlg::Msg::ShowOk(this,
                              L"Cannot show map!",
                              L"No map and/or style configured!");
      }

      map->RequestNewMap();
    }
    else if (model==GetClosedAction() &&
             GetClosedAction()->IsFinished()) {
      SaveConfig();
    }
    else if (model==locationSearchAction &&
             locationSearchAction->IsFinished()) {
      osmscout::Location location;
      bool               hasResult=false;

      LocationSearchDialog *dialog;

      dialog=new LocationSearchDialog(databaseTask);
      dialog->SetParent(this);
      if (dialog->Open()) {
        dialog->EventLoop();
        dialog->Close();

        hasResult=dialog->HasResult();
        if (dialog->HasResult()) {
          location=dialog->GetResult();
        }
      }

      delete dialog;

      if (hasResult) {
        map->ShowReference(location.references.front(),
                           osmscout::Magnification::magVeryClose);
      }
    }
    else if (model==routeAction &&
             routeAction->IsFinished()) {
      RouteDialog *dialog;

      dialog=new RouteDialog(databaseTask);
      dialog->SetParent(this);
      if (dialog->Open()) {
        dialog->EventLoop();
        dialog->Close();
      }

      delete dialog;
    }
    else if (model==settingsAction &&
             settingsAction->IsFinished()) {
      Lum::Def::PropGroup           *props=new Lum::Def::PropGroup();
      std::wstring                  label;
      osmscout::AreaSearchParameter searchParameter;

      label=L"DPI";
      label+=L" ("+Lum::Base::NumberToWString(dpi->GetMinAsUnsignedLong())+L"-"+Lum::Base::NumberToWString(dpi->GetMaxAsUnsignedLong())+L")";
      props->Number(Lum::Def::Number(Lum::Def::Desc(label),dpi));

      label=L"Maximum nodes";

      if (searchParameter.GetMaximumNodes()==std::numeric_limits<unsigned long>::max()) {
        label+=L" (unlimited)";
      }
      else {
        label+=L" ("+Lum::Base::NumberToWString(searchParameter.GetMaximumNodes())+L")";
      }
      props->Number(Lum::Def::Number(Lum::Def::Desc(label),maxNodes));

      label=L"Maximum ways";
      if (searchParameter.GetMaximumWays()==std::numeric_limits<unsigned long>::max()) {
        label+=L" (unlimited)";
      }
      else {
        label+=L" ("+Lum::Base::NumberToWString(searchParameter.GetMaximumWays())+L")";
      }
      props->Number(Lum::Def::Number(Lum::Def::Desc(label),maxWays));

      label=L"Maximum areas";
      if (searchParameter.GetMaximumAreas()==std::numeric_limits<unsigned long>::max()) {
        label+=L" (unlimited)";
      }
      else {
        label+=L" ("+Lum::Base::NumberToWString(searchParameter.GetMaximumAreas())+L")";
      }
      props->Number(Lum::Def::Number(Lum::Def::Desc(label),maxAreas));

      props->Boolean(Lum::Def::Boolean(Lum::Def::Desc(L"Optimize ways"),optimizeWays));
      props->Boolean(Lum::Def::Boolean(Lum::Def::Desc(L"Optimize areas"),optimizeAreas));

      Lum::Dlg::Properties::Show(this,props);

      map->RequestNewMap();

      delete props;

    }
    else if (model==debugFlushCacheAction && debugFlushCacheAction->IsFinished()) {
      database->FlushCache();
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


class Main : public Lum::GUIApplication<MainDialog>
{
public:
  bool Initialize()
  {
#if defined(APP_DATADIR)
    Lum::Manager::FileSystem::Instance()->SetApplicationDataDir(Lum::Base::StringToWString(APP_DATADIR));
#endif

    osmscout::DatabaseParameter databaseParameter;
    osmscout::RouterParameter   routerParameter;

    databaseParameter.SetDebugPerformance(true);

    database=new osmscout::Database(databaseParameter);
    router=new osmscout::Router(routerParameter);

    jobFinishedAction=new Lum::Model::Action();

    info.SetProgram(Lum::Base::StringToWString(PACKAGE_NAME));
    info.SetVersion(Lum::Base::StringToWString(PACKAGE_VERSION));
    info.SetDescription(_(L"ABOUT_DESC",L"A simple OSM map viewer..."));
    info.SetAuthor(L"Tim Teulings");
    info.SetContact(L"Tim Teulings <tim@teulings.org>");
    info.SetCopyright(L"(c) 2009, Tim Teulings");
    info.SetLicense(L"GNU Public License");

    if (!Lum::GUIApplication<MainDialog>::Initialize()) {
      return false;
    }

    databaseTask=new DatabaseTask(database,
                                  router,
                                  jobFinishedAction);

    databaseTask->Start();

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

    Lum::GUIApplication<MainDialog>::Cleanup();

    if (router->IsOpen()) {
      router->Close();
    }
    delete router;
    router=NULL;

    if (database->IsOpen()) {
      database->Close();
    }

    delete database;
    database=NULL;
  }
};


LUM_MAIN(Main,L"TravelJinni")

