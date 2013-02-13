//
//  OSMScoutView.m
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 18/08/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import "OSMScoutView.h"

@implementation OSMScoutView

// This should point to OSM data generated with the OSMScout Import tool
#define OSMSCOUTDATA @"/Users/vlad/Desktop/japon"
// The center of the displayed map
#define LATITUDE 35.68068
#define LONGITUDE 139.76339
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
    trackingArea = [[NSTrackingArea alloc]
                    initWithRect:self.frame
                    options:NSTrackingMouseMoved|NSTrackingInVisibleRect|NSTrackingActiveInActiveApp
                    owner:self userInfo:nil];
    [self addTrackingArea: trackingArea];
    [self defaults];
}

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
    }
    
    return self;
}

- (void)drawRect:(NSRect)rect {
    NSImage* img = [[NSImage alloc] initWithSize:rect.size];
    [img lockFocusFlipped:YES];
    CGContextRef cg = [[NSGraphicsContext currentContext] graphicsPort];
    [osmScout drawMapTo:cg location:_location zoom:_zoom size:rect.size];
    [img unlockFocus];    
    [img drawAtPoint:NSMakePoint(0.0, 0.0) fromRect:rect operation:NSCompositeCopy fraction:1.0];

}

- (CLLocationCoordinate2D) location {
    return _location;
}

-(void)setLocation:(CLLocationCoordinate2D)location {
    if(_location.latitude != location.latitude && _location.longitude != location.longitude){
        _location = location;
        [self setNeedsDisplay: YES];
    }
}

- (double)zoom {
    return _zoom;
}

-(void)setZoom:(double)zoom {
    if(_zoom != zoom){
        _zoom = zoom;
        [self setNeedsDisplay: YES];
    }
}

- (void)mouseEntered:(NSEvent *)evt {
}

- (void)mouseExited:(NSEvent *)evt {
}

- (void)mouseMoved:(NSEvent *)evt {
    //CGPoint pt = [self convertPoint: [evt locationInWindow] fromView: nil];
    _location.latitude +=0.0002*[evt deltaY];
    _location.longitude +=-0.0002*[evt deltaX];
    //CLLocationCoordinate2D loc = [self.contents pixelToLatLong:pt];
    [self setNeedsDisplay:YES];
}

@end
