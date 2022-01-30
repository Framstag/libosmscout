//
//  AppDelegate.m
//  OSMScoutiOS
//
//  Created by Vladimir Vyskocil on 20/09/12.
//  Copyright (c) 2012 libosmscout. All rights reserved.
//

#import "AppDelegate.h"

#import "OSMScoutIOSViewController.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    // Override point for customization after application launch.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        self.viewController = [[OSMScoutIOSViewController alloc] initWithNibName:@"OSMScoutIOSViewController_iPhone" bundle:nil];
    } else {
        self.viewController = [[OSMScoutIOSViewController alloc] initWithNibName:@"OSMScoutIOSViewController_iPad" bundle:nil];
    }
    self.window.rootViewController = self.viewController;
    [self.window makeKeyAndVisible];
    return YES;
}

@end
