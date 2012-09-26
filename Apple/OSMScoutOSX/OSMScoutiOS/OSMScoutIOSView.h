//
//  OSMScoutIOSView.h
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 22/09/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "OSMScout.h"

@interface OSMScoutIOSView : UIView {
    CLLocationCoordinate2D  _location;
    double                  _zoom;
    OSMScout                *osmScout;
}

@property (nonatomic) CLLocationCoordinate2D location;
@property double zoom;

@end
