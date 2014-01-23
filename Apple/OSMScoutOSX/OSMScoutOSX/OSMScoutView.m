//
//  OSMScoutView.m
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 18/08/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import "OSMScoutView.h"
#import "OSMScoutMKTileOverlay.h"

@implementation OSMScoutView

// This should point to OSM data generated with the OSMScout Import tool
#define OSMSCOUTDATA @"/Users/vlad/Desktop/France"
// The center of the displayed map
#define LATITUDE 43.694417
#define LONGITUDE 7.279332
// The zoom level
#define ZOOM 16

-(void)defaults {
    [self setCenterCoordinate:CLLocationCoordinate2DMake(LATITUDE, LONGITUDE)];
    [self setRegion:MKCoordinateRegionMakeWithDistance(self.centerCoordinate, 2000, 2000)];
    self.delegate = self;

    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    OSMScoutMKTileOverlay *overlay = [[OSMScoutMKTileOverlay alloc] initWithURLTemplate: nil];
    overlay.canReplaceMapContent = YES;
    overlay.minimumZ = 1;
    overlay.maximumZ = 21;
    overlay.geometryFlipped = YES;
    overlay.path = OSMSCOUTDATA /*[paths objectAtIndex:0]*/;
    [self insertOverlay:overlay atIndex:0 level:MKOverlayLevelAboveLabels];
    tileOverlay = overlay;
    [self insertOverlay:tileOverlay atIndex:0 level:MKOverlayLevelAboveLabels];
}

- (void)awakeFromNib {
	[super awakeFromNib];
    [self defaults];
}

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
    }
    
    return self;
}

#pragma mark - MKMapViewDelegate

- (MKOverlayRenderer *)mapView:(MKMapView *)mapView rendererForOverlay:(id<MKOverlay>)overlay {
    if ([overlay isKindOfClass:[MKTileOverlay class]]) {
        return [[MKTileOverlayRenderer alloc] initWithTileOverlay:overlay];
    } else {
        return nil;
    }
}

@end
