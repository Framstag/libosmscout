package osm.scout.benchmark;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;

import osm.scout.Database;
import osm.scout.MapData;
import osm.scout.MapPainterCanvas;
import osm.scout.MapParameter;
import osm.scout.MercatorProjection;
import osm.scout.ObjectTypeSets;
import osm.scout.Projection;
import osm.scout.StyleConfig;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.AsyncTask;

public class BenchmarkAsyncTask extends AsyncTask<Void, Integer, Boolean> {
	
	private BenchmarkProgressListener mListener;
	
	public double mMinLon;
	public double mMaxLon;
	public double mMinLat;
	public double mMaxLat;
	public int mMinZoom;
	public int mMaxZoom;
	public int mWidth;
	public int mHeight;
	
	private Database mDatabase;
	private ObjectTypeSets mTypeSets;
	private StyleConfig mStyleConfig;
	private MercatorProjection mProjection;
	private MapParameter mMapParameter;
	private MapPainterCanvas mMapPainterCanvas;
	private MapData mMapData;
	
	private Bitmap mBitmap;
	private Canvas mCanvas;
	
	public BenchmarkAsyncTask(BenchmarkProgressListener listener, Database database,
			StyleConfig styleConfig, MapParameter mapParameter) {
		
		mListener=listener;
		
		mDatabase=database;
		
		mStyleConfig=styleConfig;
		
		mProjection=new MercatorProjection();
		
		mMapParameter=mapParameter;
		
		mMapPainterCanvas=new MapPainterCanvas();
	}
	
	@Override
	protected void onPreExecute () {
		
		int totalCount=0;
			
		for(int zoom=mMinZoom; zoom<=mMaxZoom; zoom++) {
			
			int xTileStart=Projection.lon2tilex(mMinLon, zoom);
		    int xTileEnd=Projection.lon2tilex(mMaxLon, zoom);
		    int xTileCount=xTileEnd-xTileStart+1;

		    int yTileStart=Projection.lat2tiley(mMaxLat, zoom);
		    int yTileEnd=Projection.lat2tiley(mMinLat, zoom);
		    int yTileCount=yTileEnd-yTileStart+1;
		    
		    totalCount+=(xTileCount*yTileCount);
		}
		
		mBitmap=Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
		
		mCanvas=new Canvas(mBitmap);
		
		mListener.onStart(totalCount);		
	}
	
	@Override
	protected Boolean doInBackground(Void... params) {
		
		int count=0;
		
		for(int zoom=mMinZoom; zoom<=mMaxZoom; zoom++) {
			
			mTypeSets=mStyleConfig.getObjectTypesWithMaxMag(Math.pow(2.0, zoom));	    
			
			int xTileStart=Projection.lon2tilex(mMinLon, zoom);
		    int xTileEnd=Projection.lon2tilex(mMaxLon, zoom);
		    
		    int yTileStart=Projection.lat2tiley(mMaxLat, zoom);
		    int yTileEnd=Projection.lat2tiley(mMinLat, zoom);
		    
		    for (int y=yTileStart; y<=yTileEnd; y++) {
		    	
		    	for (int x=xTileStart; x<=xTileEnd; x++) {
		    		
		    		if (isCancelled()) {
		    			
		    			mListener.onEnd();
		    			
		    			return Boolean.valueOf(false);		    			
		    		}
		    		
		    		double lon=(Projection.tilex2long(x, zoom)+Projection.tilex2long(x+1, zoom))/2.0;
		    		double lat=(Projection.tiley2lat(y, zoom)+Projection.tiley2lat(y+1, zoom))/2.0;
		    		
		    		mProjection.set(lon, lat, Math.pow(2.0, zoom), mWidth, mHeight);
		    		
		    		int t1=getObjects();
		    		
		    		getGroundTiles();
		    		
		    		int t2=drawMap();
		    		
		    		count++;
					
		    		Integer[] progress=new Integer[6];
				    
		    		progress[0]=count;
		    		progress[1]=zoom;
				    progress[2]=x;
				    progress[3]=y;
				    progress[4]=t1;
				    progress[5]=t2;
				    
				    publishProgress(progress);
				    
				    /*
				    String fileName="/mnt/sdcard/map/tiles/";
				    
				    fileName+=String.format("tile%05d.png", count);
				    
				    try {
				    	
						FileOutputStream out = new FileOutputStream(fileName);
						
						mBitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
						
					} catch (FileNotFoundException e) {
						
					}
					*/
		    	}		    	
		    }			
		}
		
		mListener.onEnd();
		
		return Boolean.valueOf(true);
	}
	
	protected void onProgressUpdate(Integer... progress) {
		
		mListener.updateProgress(progress);
	}
	
	private int getObjects() {
		
		long start=System.nanoTime();
		
		mMapData=mDatabase.getObjects(mTypeSets, mProjection);
		
		long stop=System.nanoTime();
		
		return (int)((stop-start)/1000000);
	}

	private int getGroundTiles() {
		
		long start=System.nanoTime();
		
		if (mMapParameter.getRenderSeaLand()) {
		
			mDatabase.getGroundTiles(mProjection, mMapData);    		
		}
		
		long stop=System.nanoTime();
		
		return (int)((stop-start)/1000000);
	}

	
	private int drawMap() {
		
		long start=System.nanoTime();
		
		mMapPainterCanvas.drawMap(mStyleConfig, mProjection, mMapParameter, mMapData, mCanvas);
		
		long stop=System.nanoTime();
		
		return (int)((stop-start)/1000000);
	}
}
