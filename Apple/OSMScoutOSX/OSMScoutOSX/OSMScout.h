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
#include <osmscout/RoutePostprocessor.h>
#include <osmscout/StyleConfigLoader.h>
#include <osmscout/MapPainterIOS.h>
#include <osmscout/MapService.h>
#include <osmscout/LocationService.h>

namespace osmscout {
    class MyBreaker : public Breaker {
    public:
        MyBreaker() : osmscout::Breaker(), aborted(false){}
        bool Break(){
            aborted = true;
            return true;
        }
        bool IsAborted() const {
            return aborted;
        }
        void Reset() {
            aborted = false;
        }
    private:
        bool aborted;
    };
    
    class OSMScoutCpp {
    private:
        std::string         map;
        TypeSet             types;
        RoutingProfile      *routingProfile;
        DatabaseParameter   databaseParameter;
        Database            database;
        bool                isDatabaseOpened;
        StyleConfig         *styleConfig;
        MapPainterIOS       *mapPainter;
        MercatorProjection  projection;
        MapParameter        drawParameter;
        MyBreaker           *drawBreaker;
        double              loadedLatMin;
        double              loadedLonMin;
        double              loadedLatMax;
        double              loadedLonMax;
        Magnification       loadedMagnification;
        AreaSearchParameter searchParameter;
        MapService          mapService;
        MapData             data;
        bool                isMapPainterConfigured;

    public:
        OSMScoutCpp(const char *cDir);
        virtual ~OSMScoutCpp();
        void abortDrawing();
        void drawingBreakerReset();
        bool initDraw(size_t dpi);
        void drawMap(CGContextRef paintCG, double lat, double lon, double zoom, size_t width, size_t height);
    };
}
#endif

#import <CoreLocation/CLLocation.h>
@interface OSMScout : NSObject {
}
+(OSMScout *)OSMScoutWithPath:(NSString *)path dpi:(size_t)dpi;
-(id)initWithPath:(NSString *)path dpi:(size_t)dpi;
-(void)drawMapTo: (CGContextRef) cg lat:(CLLocationDegrees)lat lon:(CLLocationDegrees)lon zoom: (double) zoom width: (CGFloat) width height: (CGFloat) height;

@end

#endif
