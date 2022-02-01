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

// The OpenStreetMap imported data used to draw the map are provided by the map.osmscout folder
// in the Resources of the project, by default this is the Greater London from GeoFabrik
// (https://download.geofabrik.de/europe/great-britain/england/greater-london.html)

// The center of the displayed map
#define LATITUDE 51.5102
#define LONGITUDE -0.1024
// The zoom level
#define ZOOM 16

-(void)defaults {
    CLLocationCoordinate2D centerCoordinate = CLLocationCoordinate2DMake(LATITUDE, LONGITUDE);
    MKCoordinateSpan span = MKCoordinateSpanMake(0, 360/pow(2, ZOOM)*self.frame.size.width/256);
    [self setRegion:MKCoordinateRegionMake(centerCoordinate, span) animated:NO];
    self.delegate = self;
    OSMScoutMKTileOverlay *overlay = [[OSMScoutMKTileOverlay alloc] initWithURLTemplate: nil];
    NSString *path = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/map.osmscout"];
    OSMScoutMKTileOverlay.path = path;    
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
