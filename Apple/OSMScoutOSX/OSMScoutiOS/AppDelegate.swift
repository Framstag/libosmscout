//
//  AppDelegate.swift
//  OSMScoutiOS
//
//  Created by Vladimir Vyskocil on 30/12/2023.
//  Copyright © 2023 libosmscout. All rights reserved.
//

import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    var window : UIWindow? = nil
    var viewController : UIViewController! = nil

    
    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey : Any]? = nil) -> Bool {
        self.window = UIWindow(frame: UIScreen.main.bounds)
        
        // Override point for customization after application launch.
        if UIDevice.current.userInterfaceIdiom == .phone {
            viewController = UIViewController(nibName: "OSMScoutIOSViewController_iPhone", bundle: nil)
        } else {
            viewController = UIViewController(nibName:"OSMScoutIOSViewController_iPad", bundle:nil)
        }
        window?.rootViewController = viewController
        window?.makeKeyAndVisible()
        
        
        return true
    }
    
}
