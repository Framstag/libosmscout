//
//  OSMScoutIOSView.m
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 22/09/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import "OSMScoutIOSView.h"
#import "OSMScoutMKTileOverlay.h"

@implementation OSMScoutIOSView

// This should reference OSM data generated with the OSMScout Import tool
// used when targeting the iPhone simulator 
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
#if TARGET_IPHONE_SIMULATOR
    overlay.path = OSMSCOUTDATA;
#else
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    overlay.path = [paths objectAtIndex:0];
#endif
    overlay.canReplaceMapContent = YES;
    overlay.minimumZ = 1;
    overlay.maximumZ = 21;
    overlay.geometryFlipped = YES;
    [self insertOverlay:overlay atIndex:0 level:MKOverlayLevelAboveLabels];
    tileOverlay = overlay;
    [self insertOverlay:tileOverlay atIndex:0 level:MKOverlayLevelAboveLabels];
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

#pragma mark - MKMapViewDelegate

- (MKOverlayRenderer *)mapView:(MKMapView *)mapView rendererForOverlay:(id<MKOverlay>)overlay {
    if ([overlay isKindOfClass:[MKTileOverlay class]]) {
        return [[MKTileOverlayRenderer alloc] initWithTileOverlay:overlay];
    } else {
        return nil;
    }
}

@end
