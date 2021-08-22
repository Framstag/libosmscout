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

// When targeting the iPhone simulator, OSMSCOUTDATA should point to a folder with the
// files of a OSMScout database generated with the Import tool and the standard.oss
// stylesheet file.
// To test the app on a real device you should copy a OSMScout database files and a OSS
// stylesheet file to the app sharing directory using the Finder to access the app files
// (search the iPhone in Locations on Mac OSX >= 11.0 or use iTunes on earlier versions)
#define OSMSCOUTDATA @"/Users/vyskocil/Documents/OSM/Malta.osmscout"
// The center of the displayed map
#define LATITUDE 35.9449
#define LONGITUDE 14.3796
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
