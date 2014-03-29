//
//  RouteOSMScout.cpp
//  MyFreemap
//
//  Created by Vladimir Vyskocil on 27/02/12.
//  Copyright (c) 2012 Bleuazur. All rights reserved.
//

#include <iostream>
#include <iomanip>
#include <cmath>

#include "osmscout/MapPainterIOS.h"
#include "OSMScout.h"

namespace osmscout {
    
    OSMScoutCpp::OSMScoutCpp(const char *cDir) : database(databaseParameter),map(cDir),styleConfig(NULL),isMapPainterConfigured(false),loadedMagnification(0),loadedLatMax(0),loadedLonMax(0),loadedLatMin(0),loadedLonMin(0){
    }
    
    OSMScoutCpp::~OSMScoutCpp(){
        database.Close();
    }

    bool OSMScoutCpp::initDraw(size_t dpi) {
        if (!database.IsOpen() && !database.Open(map.c_str())) {
            std::cerr << "Cannot open database" << std::endl;
            
            return false;
        }
        
        if (!styleConfig){
            std::string style = map.c_str();
            style += "/standard.oss";
            styleConfig=new StyleConfig(database.GetTypeConfig());
            if (!LoadStyleConfig(style.c_str(),*styleConfig)) {
                std::cerr << "Cannot open style" << std::endl;
                return false;
            }
        }
        
        if(!isMapPainterConfigured){
            mapPainter = new osmscout::MapPainterIOS();
            drawParameter.SetFontName("GillSans");
            drawParameter.SetFontSize(1.5);
            std::list<std::string> paths;
            NSString *path = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/"];
            paths.push_back([path UTF8String]);
            drawParameter.SetIconPaths(paths);
            drawParameter.SetPatternPaths(paths);
            drawParameter.SetDPI(dpi);
            drawParameter.SetRenderSeaLand(true);
            drawParameter.SetDebugPerformance(false);
            searchParameter.SetMaximumNodes(30000);
            drawBreaker = new MyBreaker();
            drawParameter.SetBreaker(drawBreaker);
            searchParameter.SetBreaker(drawBreaker);
            isMapPainterConfigured = true;
        }
        return true;
    }
    
    void OSMScoutCpp::drawMap(CGContextRef paintCG, double lat, double lon, double zoom, size_t width, size_t height){
        assert(isMapPainterConfigured);
        projection.Set(lon, lat, zoom, (width+1), (height+1));
        
        TypeSet              nodeTypes;
        std::vector<TypeSet> wayTypes;
        TypeSet              areaTypes;
        
        styleConfig->GetNodeTypesWithMaxMag(projection.GetMagnification(), nodeTypes);
        styleConfig->GetWayTypesByPrioWithMaxMag(projection.GetMagnification(), wayTypes);
        styleConfig->GetAreaTypesWithMaxMag(projection.GetMagnification(), areaTypes);
        
        double lon1,lat1,lon2,lat2;
        projection.GetDimensions(lon1, lat1, lon2, lat2);
        if(projection.GetMagnification() == loadedMagnification &&
           !(lon2>loadedLonMax ||
             lon1<loadedLonMin ||
             lat2>loadedLatMax ||
             lat1<loadedLatMin)){
               std::cout<<"DrawMap at ("<<lon<<","<<lat<<") using cached data"<<std::endl;
           } else {
               loadedLonMin = lon1;
               loadedLatMin = lat1;
               loadedLonMax = lon2;
               loadedLatMax = lat2;
               double deltaLon = loadedLonMax - loadedLonMin;
               loadedLonMin -= 2*deltaLon;
               loadedLonMax += 2*deltaLon;
               double deltaLat = loadedLatMax - loadedLatMin;
               loadedLatMin -= 2*deltaLat;
               loadedLatMax += 2*deltaLat;
               loadedMagnification = projection.GetMagnification();
               std::cout<<"DrawMap at ("<<lon<<","<<lat<<") loading data for ("<<loadedLonMin<<","<<loadedLatMin<<"),(" <<loadedLonMax<<","<<loadedLatMax<<")"<<std::endl;
               database.FlushCache();
               database.GetObjects(nodeTypes,
                                   wayTypes,
                                   areaTypes,
                                   loadedLonMin,
                                   loadedLatMin,
                                   loadedLonMax,
                                   loadedLatMax,
                                   loadedMagnification,
                                   searchParameter,
                                   data.nodes,
                                   data.ways,
                                   data.areas);
               
               if (drawParameter.GetRenderSeaLand()) {
                   database.GetGroundTiles(loadedLonMin,
                                           loadedLatMin,
                                           loadedLonMax,
                                           loadedLatMax,
                                           loadedMagnification,
                                           data.groundTiles);
               }
           }
        mapPainter->DrawMap(*styleConfig, projection, drawParameter, data, paintCG);
    }
    
    
    void OSMScoutCpp::abortDrawing(){
        if(drawBreaker){
            std::cout<<"abortDrawing !"<<std::endl;
            drawBreaker->Break();
        }
    }
    
    void OSMScoutCpp::drawingBreakerReset(){
        if(drawBreaker){
            std::cout<<"drawingBreakerReset"<<std::endl;
            drawBreaker->Reset();
        }
    }
    
    
}

@interface OSMScout ()
@property (nonatomic, readwrite, assign) osmscout::OSMScoutCpp *osmScout;
@end

@implementation OSMScout
@synthesize osmScout = _osmScoutCpp;

static OSMScout* osmScoutInstance = nil;
static NSString* osmScoutRootPath = @"";

+(OSMScout *)OSMScoutWithPath:(NSString *)path dpi:(size_t)dpi {
    if(!osmScoutInstance){
        osmScoutRootPath = [path copy];
        osmScoutInstance = [[self alloc] initWithPath:osmScoutRootPath dpi:dpi];
    } else {
        [osmScoutInstance drawingBreakerReset];
    }
    return osmScoutInstance;
}

-(id)initWithPath:(NSString *)path dpi:(size_t)dpi {
    if((self = [super init])){
        _osmScoutCpp = new osmscout::OSMScoutCpp([path UTF8String]);
        _osmScoutCpp->initDraw(dpi);
    }
    return self;
}
-(void)dealloc{
    delete _osmScoutCpp;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

-(void)drawMapTo: (CGContextRef)cg lat:(CLLocationDegrees)lat lon:(CLLocationDegrees)lon zoom: (double) zoom width: (CGFloat) width height: (CGFloat) height {
    _osmScoutCpp->drawMap(cg, lat, lon, zoom, width, height);
}

-(void)abortDrawing {
    _osmScoutCpp->abortDrawing();
}

-(void)drawingBreakerReset {
    _osmScoutCpp->drawingBreakerReset();
}


@end

