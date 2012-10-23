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
void OSMScoutCPP::drawMap(CGContextRef paintCG, double lat, double lon, double zoom, size_t width, size_t height){
    std::string style = map.c_str();
    style += "/standard.oss";
    
    if (!isDatabaseOpened && !database.Open(map.c_str())) {
        std::cerr << "Cannot open database" << std::endl;
        
        return;
    }
    isDatabaseOpened = YES;
    
    if (!styleConfig){
        styleConfig=new osmscout::StyleConfig(database.GetTypeConfig());
        if (!osmscout::LoadStyleConfig(style.c_str(),*styleConfig)) {
            std::cerr << "Cannot open style" << std::endl;
            return;
        }
    }
#if TARGET_OS_IPHONE
    osmscout::MercatorProjection  projection;
#else
    osmscout::ReversedYAxisMercatorProjection  projection;
#endif
    osmscout::MapParameter        drawParameter;
    osmscout::AreaSearchParameter searchParameter;
    osmscout::MapData             data;
    
    projection.Set(lon,lat,zoom,width,height);
    
    osmscout::TypeSet              nodeTypes;
    std::vector<osmscout::TypeSet> wayTypes;
    osmscout::TypeSet              areaTypes;
    
    styleConfig->GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                        nodeTypes);
    
    styleConfig->GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                             wayTypes);
    
    styleConfig->GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                        areaTypes);
    
    database.GetObjects(nodeTypes,
                        wayTypes,
                        areaTypes,
                        projection.GetLonMin(),
                        projection.GetLatMin(),
                        projection.GetLonMax(),
                        projection.GetLatMax(),
                        projection.GetMagnification(),
                        searchParameter,
                        data.nodes,
                        data.ways,
                        data.areas,
                        data.relationWays,
                        data.relationAreas);

    drawParameter.SetDPI(120);
    drawParameter.SetRenderSeaLand(true);

    if (drawParameter.GetRenderSeaLand()) {
        database.GetGroundTiles(projection.GetLonMin(),
                                projection.GetLatMin(),
                                projection.GetLonMax(),
                                projection.GetLatMax(),
                                projection.GetMagnification(),
                                data.groundTiles);
    }
    
    drawParameter.SetFontName("Helvetica-Bold");
    std::list<std::string>        paths;
    NSString *path = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/"];
    paths.push_back([path UTF8String]);
    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    
    if(!mapPainter){
        mapPainter = new osmscout::MapPainterIOS();
    }
    if (mapPainter->DrawMap(*styleConfig,projection,drawParameter,data,paintCG)) {
    }
}

    OSMScoutCPP::OSMScoutCPP(const char *cDir) : database(databaseParameter),isDatabaseOpened(NO),router(routerParameter),description(NULL),map(cDir),styleConfig(NULL),mapPainter(0) {
    }

    OSMScoutCPP::~OSMScoutCPP(){
        database.Close();
        router.Close();
    }
}

@interface OSMScout ()
@property (nonatomic, readwrite, assign) osmscout::OSMScoutCPP *osmScout;
@end

@implementation OSMScout
@synthesize osmScout = _osmScoutCPP;

-(id)initWithPath:(NSString *)path{
    if((self = [super init])){
        _osmScoutCPP = new osmscout::OSMScoutCPP([path UTF8String]);
    }
    return self;
}
-(void)dealloc{
    delete _osmScoutCPP;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

-(void)drawMapTo: (CGContextRef)cg location:(CLLocationCoordinate2D)loc zoom: (double) zoom size: (CGSize) size {
    _osmScoutCPP->drawMap(cg, loc.latitude, loc.longitude, zoom, size.width, size.height);
}

@end

