//
//  OSMScoutIOSView.h
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 22/09/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import <MapKit/MapKit.h>
#import "OSMScout.h"

@interface OSMScoutIOSView : MKMapView <MKMapViewDelegate> {
    MKTileOverlay           *tileOverlay;
}

@end
