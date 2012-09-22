//
//  OSMScout.h
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 27/02/12.
//  Copyright (c) 2012 Bleuazur. All rights reserved.
//

#ifndef OSMScout_h
#define OSMScout_h

#if __cplusplus
#include <osmscout/TypeSet.h>
#include <osmscout/Database.h>
#include <osmscout/Way.h>
#include <osmscout/Tag.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/Router.h>
#include <osmscout/RoutePostprocessor.h>
#include <osmscout/StyleConfigLoader.h>
#include <osmscout/MapPainterIOS.h>

namespace osmscout {
    class OSMScoutCPP {
    private:
        std::string                 map;
        osmscout::TypeSet           types;
        osmscout::RoutingProfile    *routingProfile;
        osmscout::DatabaseParameter databaseParameter;
        osmscout::Database          database;
        bool                        isDatabaseOpened;
        osmscout::RouterParameter   routerParameter;
        osmscout::Router            router;
        osmscout::RouteDescription  *description;
        osmscout::StyleConfig       *styleConfig;
        osmscout::MapPainterIOS     *mapPainter;
        
    public:
        OSMScoutCPP(const char *cDir);
        virtual ~OSMScoutCPP();
        void drawMap(CGContextRef paintCG, double lat, double lon, double zoom, size_t width, size_t height);
    };
}
#endif

#import <CoreLocation/CLLocation.h>
@interface OSMScout : NSObject {
}
-(id)initWithPath:(NSString *)path;
-(void)drawMapTo: (CGContextRef) cg location:(CLLocationCoordinate2D)loc zoom: (double) zoom size: (CGSize) size;

@end

#endif
