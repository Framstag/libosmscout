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
// If not defined the data would be looked at in the App Document dir
#define OSMSCOUTDATA @"/Users/vlad/Desktop/France"
// The center of the displayed map
#define LATITUDE 43.694417
#define LONGITUDE 7.279332
// The zoom level
#define ZOOM 16

-(void)defaults {
    CLLocationCoordinate2D centerCoordinate = CLLocationCoordinate2DMake(LATITUDE, LONGITUDE);
    MKCoordinateSpan span = MKCoordinateSpanMake(0, 360/pow(2, ZOOM)*self.frame.size.width/256);
    [self setRegion:MKCoordinateRegionMake(centerCoordinate, span) animated:NO];
    self.delegate = self;
    OSMScoutMKTileOverlay *overlay = [[OSMScoutMKTileOverlay alloc] initWithURLTemplate: nil];
#ifdef OSMSCOUTDATA
    overlay.path = OSMSCOUTDATA;
#else
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    overlay.path = [paths objectAtIndex:0];
#endif
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
