//
//  OSMScoutView.swift
//  OSMScoutOSX
//
//  Created by Vladimir Vyskocil on 31/12/2023.
//  Copyright © 2023 libosmscout. All rights reserved.
//

import MapKit
import CoreLocation

// The OpenStreetMap compiled data with the Import tools should be put in map.osmscout
// folder in the Resources of the project, a example of OSM source data is
// the Greater London from GeoFabrik
// (https://download.geofabrik.de/europe/great-britain/england/greater-london.html)

class OSMScoutView: MKMapView, MKMapViewDelegate {
    // The center of the displayed map
    let LATITUDE = 51.5102
    let LONGITUDE = -0.1024
    // The zoom level
    let ZOOM = 16.0
    
    var tileOverlay : MKTileOverlay! = nil
    
    func defaults() {
        showsTraffic = false
        showsBuildings = false
        pointOfInterestFilter = .excludingAll
        
        let centerCoordinate = CLLocationCoordinate2D(latitude:LATITUDE, longitude:LONGITUDE)
        let longitudeDelta =  360.0 / pow(2, ZOOM) * Double(frame.size.width/256)
        let span = MKCoordinateSpan(latitudeDelta: 0, longitudeDelta: longitudeDelta)
        
        setRegion(MKCoordinateRegion(center: centerCoordinate, span: span), animated:false)
        delegate = self
        let overlay = OSMScoutMKTileOverlay(urlTemplate: nil)
        let path = (Bundle.main.resourcePath ?? "") + "/map.osmscout"
        OSMScoutMKTileOverlay.path = path
        tileOverlay = overlay;
        insertOverlay(tileOverlay, at:0, level:.aboveLabels)
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
        defaults()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        defaults()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        defaults()
    }

    // MARK: - MKMapViewDelegate

    func mapView(_ mapView: MKMapView, rendererFor overlay: MKOverlay) -> MKOverlayRenderer {
        if let overlay  = overlay as? MKTileOverlay {
            return MKTileOverlayRenderer(tileOverlay: overlay)
        } else {
            return MKOverlayRenderer()
        }
    }
}
