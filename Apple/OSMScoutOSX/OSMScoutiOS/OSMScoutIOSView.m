//
//  OSMScoutIOSView.m
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 22/09/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import "OSMScoutIOSView.h"

@implementation OSMScoutIOSView

// This should point to OSM data generated with the OSMScout Import tool
#define OSMSCOUTDATA @"/Users/vlad/osmscout-map/OSMScout"
// The center of the displayed map
#define LATITUDE 43.694417
#define LONGITUDE 7.279332
// The zoom level
#define ZOOM 16

-(void)defaults {
    osmScout=[[OSMScout alloc] initWithPath:OSMSCOUTDATA];
    _location.latitude = LATITUDE;
    _location.longitude = LONGITUDE;
    _zoom = 1<<ZOOM;
}

- (void)awakeFromNib {
	[super awakeFromNib];
    [self defaults];
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self defaults];
    }
    return self;
}


- (void)drawRect:(CGRect)rect {
    CGContextRef cg=UIGraphicsGetCurrentContext();
    [osmScout drawMapTo:cg location:_location zoom:_zoom size:rect.size];
}

-(CLLocationCoordinate2D)location {
    return _location;
}

-(void)setLocation:(CLLocationCoordinate2D)location {
    if(_location.latitude != location.latitude && _location.longitude != location.longitude){
        _location = location;
        [self setNeedsDisplay];
    }
}

- (double)zoom {
    return _zoom;
}

-(void)setZoom:(double)zoom {
    if(_zoom != zoom){
        _zoom = zoom;
        [self setNeedsDisplay];
    }
}


@end
