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
#include "osmscout/util/Tiling.h"
#include "OSMScout.h"

namespace osmscout {
    
    OSMScoutCpp::OSMScoutCpp(const char *cDir) : database(new Database(databaseParameter)),map(cDir),styleConfig(NULL),isMapPainterConfigured(false),loadedMagnification(0),loadedLatMax(0),loadedLonMax(0),loadedLatMin(0),loadedLonMin(0),
        mapService(new MapService(database)),dpi(96.0){
    }
    
    OSMScoutCpp::~OSMScoutCpp(){
        database->Close();
    }

    bool OSMScoutCpp::initDraw(double initDpi) {
        if (!database->IsOpen() && !database->Open(map.c_str())) {
            std::cerr << "Cannot open database" << std::endl;
            
            return false;
        }
        
        if (!styleConfig){
            std::string style = map.c_str();
            style += "/standard.oss";
            styleConfig=StyleConfigRef(new StyleConfig(database->GetTypeConfig()));
            if (!styleConfig->Load(style)) {
                std::cerr << "Cannot open style" << std::endl;
                return false;
            }
        }
        
        if(!isMapPainterConfigured){
            dpi = initDpi;
            mapPainter = new osmscout::MapPainterIOS(styleConfig);
            drawParameter.SetFontName("GillSans");
            drawParameter.SetFontSize(1.5);
            std::list<std::string> paths;
            NSString *path = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES)[0];
            paths.push_back([path UTF8String]);
            NSString *imagePath = [[path stringByAppendingPathComponent: @"icons"] stringByAppendingString:@"/"];
            paths.push_back([imagePath UTF8String]);
            drawParameter.SetIconPaths(paths);
            drawParameter.SetPatternPaths(paths);
            drawParameter.SetRenderSeaLand(true);
            drawParameter.SetDebugPerformance(false);
            drawBreaker = std::shared_ptr<MyBreaker>(new MyBreaker());
            drawParameter.SetBreaker(drawBreaker);
            drawParameter.SetDropNotVisiblePointLabels(false);
            searchParameter.SetBreaker(drawBreaker);
            isMapPainterConfigured = true;
        }
        return true;
    }
    
    void OSMScoutCpp::drawMap(CGContextRef paintCG, size_t x, size_t y, double zoom, size_t width, size_t height){
        assert(isMapPainterConfigured);
        Magnification mag(zoom);
        projection.Set(OSMTileId((uint32_t)x,(uint32_t)y), mag, dpi, (width+1), (height+1));
        GeoBox boundingBox;
        projection.GetDimensions(boundingBox);
        
        std::list<osmscout::TileRef> tiles;
        mapService->LookupTiles(projection,tiles);
        mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
        data.ClearDBData();
        mapService->AddTileDataToMapData(tiles,data);
        if (drawParameter.GetRenderSeaLand()) {
            mapService->GetGroundTiles(boundingBox,
                                       mag,
                                       data.groundTiles);
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

+(OSMScout *)OSMScoutWithPath:(NSString *)path dpi:(double)dpi {
    if(!osmScoutInstance){
        osmScoutRootPath = [path copy];
        osmScoutInstance = [[self alloc] initWithPath:osmScoutRootPath dpi:dpi];
    } else {
        [osmScoutInstance drawingBreakerReset];
    }
    return osmScoutInstance;
}

-(id)initWithPath:(NSString *)path dpi:(double)dpi {
    if((self = [super init])){
        _osmScoutCpp = new osmscout::OSMScoutCpp([path UTF8String]);
        _osmScoutCpp->initDraw(dpi);
    }
    return self;
}
-(void)dealloc{
    delete _osmScoutCpp;
}

-(void)drawMapTo: (CGContextRef)cg x:(NSUInteger)x y:(NSUInteger)y zoom: (double) zoom width: (CGFloat) width height: (CGFloat) height {
    _osmScoutCpp->drawMap(cg, x, y, zoom, width, height);
}

-(void)abortDrawing {
    _osmScoutCpp->abortDrawing();
}

-(void)drawingBreakerReset {
    _osmScoutCpp->drawingBreakerReset();
}


@end

