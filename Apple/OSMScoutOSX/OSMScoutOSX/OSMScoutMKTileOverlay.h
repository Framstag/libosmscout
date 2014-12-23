//
//  OSMScoutMKTileOverlay.h
//  MyFreemap
//
//  Created by Vladimir Vyskocil on 06/11/2013.
//  Copyright (c) 2013 Bleuazur. All rights reserved.
//

#import <MapKit/MapKit.h>
#import "OSMScout.h"

@class RMFractalTileProjection;

#define kOSMScoutDefaultTileSize 256
#define kOSMScoutDefaultMinTileZoom 1
#define kOSMScoutDefaultMaxTileZoom 21
#define kOSMScoutDefaultLatLonBoundingBox ((RMSphericalTrapezium){ .northeast = { .latitude =  85, .longitude =  180 }, \
.southwest = { .latitude = -85, .longitude = -180 } })

typedef void (^OSMScoutMKTileOperationCB)(NSData *tileData, NSError *error) ;

@interface OSMScoutMKTileOperation : NSOperation {
    BOOL        executing;
    BOOL        finished;
    OSMScout    *_osmScout;
    NSInteger   _zoom;
    NSUInteger  _x;
    NSUInteger  _y;
    NSInteger   _scaleFactor;
    OSMScoutMKTileOperationCB _result;
}

-(id)initWithOsmScout: (OSMScout *)osmScout x:(NSUInteger)x y:(NSUInteger)y zoom:(NSInteger)zoom scaleFactor: (CGFloat)scaleFactor result: (OSMScoutMKTileOperationCB)result ;
@end

@interface OSMScoutMKTileOverlay : MKTileOverlay {
    NSString                 *_path;
    OSMScout                *_osmScout;
    NSOperationQueue        *_drawQueue;

}
@property (retain, nonatomic) NSString *path;

-(id)initWithURLTemplate: (NSString *)urlTemplate;

@end
