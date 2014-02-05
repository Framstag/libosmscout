//
//  OSMScoutView.h
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 18/08/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <CoreLocation/CoreLocation.h>
#import <MapKit/MapKit.h>
#import "OSMScout.h"

@interface OSMScoutView : MKMapView <MKMapViewDelegate> {
    MKTileOverlay           *tileOverlay;
}

@end
