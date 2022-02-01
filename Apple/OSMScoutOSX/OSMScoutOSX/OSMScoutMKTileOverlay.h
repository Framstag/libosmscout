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
    CGFloat     _contentScaleFactor;
    OSMScoutMKTileOperationCB _result;
}

-(id)initWithOsmScout: (OSMScout *)osmScout x:(NSUInteger)x y:(NSUInteger)y zoom:(NSInteger)zoom contentScaleFactor:(CGFloat)contentScaleFactor  result:(OSMScoutMKTileOperationCB)result;
@end

@interface OSMScoutMKTileOverlay : MKTileOverlay

@property (class, retain, nonatomic) NSString *path;
@property (class, retain, nonatomic, readonly) OSMScout *osmScout;
@property (class, retain, nonatomic, readonly) NSOperationQueue *drawQueue;

-(id)initWithURLTemplate: (NSString *)urlTemplate;

@end
