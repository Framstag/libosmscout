//
//  OSMScoutView.h
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 18/08/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <CoreLocation/CoreLocation.h>
#import "OSMScout.h"

@interface OSMScoutView : NSView {
    CLLocationCoordinate2D  _location;
    double                  _zoom;
    OSMScout                *osmScout;
    NSTrackingArea          *trackingArea;
}

@property (nonatomic) CLLocationCoordinate2D location;
@property double zoom;
@end
