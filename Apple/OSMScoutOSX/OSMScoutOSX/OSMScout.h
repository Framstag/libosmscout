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
#include <osmscout/Database.h>
#include <osmscout/Way.h>
#include <osmscout/Tag.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/MapPainterIOS.h>
#include <osmscout/MapService.h>
#include <osmscout/LocationService.h>

namespace osmscout {
    class MyBreaker : public Breaker {
    public:
        MyBreaker() : osmscout::Breaker(), aborted(false){}
        void Break(){
            aborted = true;
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
        RoutingProfile      *routingProfile;
        DatabaseParameter   databaseParameter;
        DatabaseRef         database;
        bool                isDatabaseOpened;
        StyleConfigRef      styleConfig;
        MapPainterIOS       *mapPainter;
        TileProjection      projection;
        MapParameter        drawParameter;
        std::shared_ptr<MyBreaker> drawBreaker;
        double              loadedLatMin;
        double              loadedLonMin;
        double              loadedLatMax;
        double              loadedLonMax;
        Magnification       loadedMagnification;
        double              dpi;
        AreaSearchParameter searchParameter;
        MapServiceRef       mapService;
        MapData             data;
        bool                isMapPainterConfigured;

    public:
        OSMScoutCpp(const char *cDir);
        virtual ~OSMScoutCpp();
        void abortDrawing();
        void drawingBreakerReset();
        bool initDraw(double dpi);
        void drawMap(CGContextRef paintCG, size_t x, size_t y, double zoom, size_t width, size_t height);
    };
}
#endif

#import <CoreLocation/CLLocation.h>
@interface OSMScout : NSObject {
}
+(OSMScout *)OSMScoutWithPath:(NSString *)path dpi:(double)dpi;
-(id)initWithPath:(NSString *)path dpi:(double)dpi;
-(void)drawMapTo: (CGContextRef) cg x:(NSUInteger)lat y:(NSUInteger)y zoom: (double) zoom width: (CGFloat) width height: (CGFloat) height;

@end

#endif
