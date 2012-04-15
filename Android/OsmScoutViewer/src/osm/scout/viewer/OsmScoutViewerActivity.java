/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

package osm.scout.viewer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.View;
import android.widget.ZoomControls;

import osm.scout.Database;
import osm.scout.GeoBox;
import osm.scout.GeoPos;
import osm.scout.MapData;
import osm.scout.OsmScoutLib;
import osm.scout.OsmScoutMapEventListener;
import osm.scout.MercatorProjection;
import osm.scout.OsmScoutMapView;
import osm.scout.StyleConfig;

public class OsmScoutViewerActivity extends Activity implements OsmScoutMapEventListener {
		
	// OsmScout objects
	private Database mDatabase=null;
	private MercatorProjection mProjection=null;
	private StyleConfig mStyleConfig=null;
	
	// GUI objects
	private OsmScoutMapView	mOsmScoutMapView=null;
	private ZoomControls mZoomControls=null;
	
	// Map (Database) path 
	private static String mMapPath="/mnt/sdcard/map";
	
	// Geographic position of the user's screen down action
	GeoPos mActionDownPos;
	
	// Zoom In-Out Scale factor
	private final static double MAG_SCALE=1.5;
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
    	
        super.onCreate(savedInstanceState);
        
		////////////////////////////////////////////////////////
		// Load of JNI libraries
		
		try {
			OsmScoutLib.loadLibrary();
			
		}
		catch(UnsatisfiedLinkError ule) {
			
			finishActivity(ule.getMessage());			
			return;
		}
		
		////////////////////////////////////////////////////////
		// Creation of OsmScout objects
		
		mDatabase=new Database();
		
        if (!mDatabase.open(mMapPath)) {
        	
        	finishActivity("Error opening database in <"+mMapPath+">");
        	return;
        }
        
        mStyleConfig=new StyleConfig();
        
        if (!mStyleConfig.loadStyleConfig(mMapPath+"/standard.oss")) {
        	
        	finishActivity("Error loading style config");
        	return;
        }
        
        // Get map boundaries
        GeoBox mapBounds=mDatabase.getBoundingBox();
        
        // Get map center
        GeoPos mapCenter=mapBounds.getCenter();
        
        mProjection=new MercatorProjection();
        
        // Set projection position to the center of the map and zoom 12 
        mProjection.setPos(mapCenter, Math.pow(2.0, 12.0));
    	
        // Create controls from resource layout
        setContentView(R.layout.main);
        
        // Get reference to the OsmScoutMapView
        mOsmScoutMapView=(OsmScoutMapView)findViewById(R.id.osmScoutMapView);
        
        // Set current activity as the listener of the OsmScoutMapView events 
        mOsmScoutMapView.setMapEventListener(this);
        
        // Get reference to the Zoom In-Out Controls        
        mZoomControls=(ZoomControls)findViewById(R.id.zoomControls);
        
        // Typical 
        mZoomControls.setZoomSpeed(200);
        
        // Define Zoom-In action
        mZoomControls.setOnZoomInClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	
            	mProjection.setMagnification(
            			mProjection.getMagnification()*MAG_SCALE);
            	
            	updateMap();
            }
        });
        
        // Define Zoom-Out action
        mZoomControls.setOnZoomOutClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	
            	mProjection.setMagnification(
            			mProjection.getMagnification()/MAG_SCALE);
            	
            	updateMap();                   
            }
        });
    }
    
    /** Override onConfigurationChanged() to prevent onCreate() calls when device rotates. */
    @Override
	public void onConfigurationChanged(Configuration newConfig) {
		
		super.onConfigurationChanged(newConfig);
	}
    
    /** Shows error message dialog and finishes activity */ 
    private void finishActivity(String errorMessage) {
    	
    	AlertDialog.Builder builder = new AlertDialog.Builder(this);
    
	    builder.setMessage(errorMessage);
	    builder.setCancelable(false);
	    builder.setPositiveButton("Exit", new DialogInterface.OnClickListener() {
	               public void onClick(DialogInterface dialog, int id) {
	            	   
	                    OsmScoutViewerActivity.this.finish();
	               }
	           });
	    
	    AlertDialog alert=builder.create();
	    
	    alert.show();
    }
    
    /** Updates map data (objects) and redraws map */ 
    private void updateMap() {
    
    	// Get database objects for current projection
    	MapData mMapData=mDatabase.getObjects(mProjection);
    
    	// Redraws map
    	mOsmScoutMapView.drawMap(mStyleConfig, mProjection, mMapData);
    }

	public void onMapSizeChanged(int width, int height) {
		
		mProjection.setSize(width, height);
		
		updateMap();
	}
	
	public void onActionDown(int x, int y) {
		
		mActionDownPos=mProjection.pixelToGeo(x, y);
	}
	
	public void onActionMove(int x, int y) {
		
		GeoPos movePos=mProjection.pixelToGeo(x, y);
		
		double lonDiff=mActionDownPos.getLon()-movePos.getLon();
		double latDiff=mActionDownPos.getLat()-movePos.getLat();
		
		mProjection.setPos(
				mProjection.getLon()+lonDiff,
				mProjection.getLat()+latDiff);
		
		updateMap();
	}	
}
