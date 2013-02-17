package osm.scout.benchmark;

import java.io.FileWriter;
import java.io.IOException;
import java.util.Vector;

import osm.scout.Database;
import osm.scout.MapParameter;
import osm.scout.OsmScoutLib;
import osm.scout.StyleConfig;
import android.os.Bundle;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.SharedPreferences;
import android.view.Display;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.widget.Button;
import android.widget.EditText;

public class OsmScoutBenchmarkActivity extends Activity implements OnClickListener, OnFocusChangeListener, BenchmarkProgressListener, OnCancelListener {
	
	Button mStartButton;
	
	EditText mMinLatEditText;
	EditText mMaxLatEditText;
	EditText mMinLonEditText;
	EditText mMaxLonEditText;
	EditText mMinZoomEditText;
	EditText mMaxZoomEditText;
	EditText mWidthEditText;
	EditText mHeightEditText;
	
	float mMinLat, mMaxLat;
	float mMinLon, mMaxLon;
	int mMinZoom, mMaxZoom;
	int mWidth, mHeight;
	
	int mTotalCount;
	
	int MAX_ZOOM_LEVEL=18;
	
	int mT1MinValues[]=new int[MAX_ZOOM_LEVEL+1];
	int mT1MaxValues[]=new int[MAX_ZOOM_LEVEL+1];
	int mT1TimeSum[]=new int[MAX_ZOOM_LEVEL+1];
	
	int mT2MinValues[]=new int[MAX_ZOOM_LEVEL+1];
	int mT2MaxValues[]=new int[MAX_ZOOM_LEVEL+1];
	int mT2TimeSum[]=new int[MAX_ZOOM_LEVEL+1];
	
	int mTileCount[]=new int[MAX_ZOOM_LEVEL+1];
	
	BenchmarkAsyncTask mTask;
	
	ProgressDialog mProgressDialog;
	
	private static String mMapPath="/mnt/sdcard/map";
		
	private static String mIconsPath=mMapPath+"/icons";
		
	Database mDatabase;
	StyleConfig mStyleConfig;
	MapParameter mMapParameter;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.activity_osm_scout_benchmark);
		
		mStartButton=(Button)findViewById(R.id.buttonStart);
		mStartButton.setOnClickListener(this);
		
		readValues();
		
		checkValues();
		
		mMinLatEditText=(EditText)findViewById(R.id.editTextMinLat);
		mMinLatEditText.setOnFocusChangeListener(this);
		
		mMaxLatEditText=(EditText)findViewById(R.id.editTextMaxLat);
		mMaxLatEditText.setOnFocusChangeListener(this);
		
		mMinLonEditText=(EditText)findViewById(R.id.editTextMinLon);
		mMinLonEditText.setOnFocusChangeListener(this);
		
		mMaxLonEditText=(EditText)findViewById(R.id.editTextMaxLon);
		mMaxLonEditText.setOnFocusChangeListener(this);
		
		mMinZoomEditText=(EditText)findViewById(R.id.editTextMinZoom);
		mMinZoomEditText.setOnFocusChangeListener(this);
		
		mMaxZoomEditText=(EditText)findViewById(R.id.editTextMaxZoom);
		mMaxZoomEditText.setOnFocusChangeListener(this);
		
		mWidthEditText=(EditText)findViewById(R.id.editTextWidth);
		mWidthEditText.setOnFocusChangeListener(this);
		
		mHeightEditText=(EditText)findViewById(R.id.editTextHeight);
		mHeightEditText.setOnFocusChangeListener(this);
		
		updateControls();
		
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

		mStyleConfig=new StyleConfig(mDatabase.getTypeConfig());

		if (!mStyleConfig.loadStyleConfig(mMapPath+"/standard.oss")) {

			finishActivity("Error loading style config");
			return;
		}

		// Set icons and pattern paths in MapParameter
		mMapParameter=new MapParameter();

		// Activate sea/land rendering
		mMapParameter.setRenderSeaLand(true);

		Vector<String> iconPaths=new Vector<String>();
		iconPaths.add(mIconsPath); 

		mMapParameter.setIconPaths(iconPaths);
		mMapParameter.setPatternPaths(iconPaths);
	}

	/** Shows error message dialog and finishes activity */ 
	private void finishActivity(String errorMessage) {
	
		AlertDialog.Builder builder = new AlertDialog.Builder(this);

		builder.setMessage(errorMessage);
		builder.setCancelable(false);
		builder.setPositiveButton("Exit", new DialogInterface.OnClickListener() {
               public void onClick(DialogInterface dialog, int id) {
            	   
            	   finish();
               }
           });
    
    	AlertDialog alert=builder.create();
    
    	alert.show();
	}

	@Override
	public void onClick(View v) {
		
		if (v==mStartButton) {
			
			mTask=new BenchmarkAsyncTask(this, mDatabase, mStyleConfig,
					mMapParameter);
			
			mTask.mMinLon=mMinLon;
			mTask.mMaxLon=mMaxLon;
			mTask.mMinLat=mMinLat;
			mTask.mMaxLat=mMaxLat;
			mTask.mMinZoom=mMinZoom;
			mTask.mMaxZoom=mMaxZoom;
			mTask.mWidth=mWidth;
			mTask.mHeight=mHeight;
			
			mTask.execute();
		}		
	}
	
	private void readValues() {
		
		SharedPreferences prefs=getSharedPreferences("OsmScoutBenchmark", MODE_PRIVATE);
		
		mMinLat=prefs.getFloat("MinLat", 1);
		mMaxLat=prefs.getFloat("MaxLat", 2);
		
		mMinLon=prefs.getFloat("MinLon", 1);
		mMaxLon=prefs.getFloat("MaxLon", 2);
		
		mMinZoom=prefs.getInt("MinZoom", 14);
		mMaxZoom=prefs.getInt("MaxZoom", 18);
		
		Display display = getWindowManager().getDefaultDisplay();
		
		mWidth=prefs.getInt("Width", display.getWidth());
		mHeight=prefs.getInt("Height", display.getHeight());		
	}
	
	private boolean checkValues() {		
		
		mMinLat=Math.min(mMinLat, 90);
		mMinLat=Math.max(mMinLat, -90);
		
		mMaxLat=Math.min(mMaxLat, 90);
		mMaxLat=Math.max(mMaxLat, -90);
		
		if (mMinLat>mMaxLat) {
			
			float aux=mMinLat;
			
			mMinLat=mMaxLat;
			
			mMaxLat=aux;
		}
		
		mMinLon=Math.min(mMinLon, 180);
		mMinLon=Math.max(mMinLon, -180);
		
		mMaxLon=Math.min(mMaxLon, 180);
		mMaxLon=Math.max(mMaxLon, -180);
		
		if (mMinLon>mMaxLon) {
			
			float aux=mMinLon;
			
			mMinLon=mMaxLon;
			
			mMaxLon=aux;
		}

		mMinZoom=Math.min(mMinZoom, MAX_ZOOM_LEVEL);
		mMinZoom=Math.max(mMinZoom, 1);
		
		mMaxZoom=Math.min(mMaxZoom, MAX_ZOOM_LEVEL);
		mMaxZoom=Math.max(mMaxZoom, 1);
		
		if (mMinZoom>mMaxZoom) {
			
			int aux=mMinZoom;
			
			mMinZoom=mMaxZoom;
			
			mMaxZoom=aux;
		}
		
		return true;		
	}
	
	private void readControls() {
		
		try {
			mMinLat=Float.valueOf(mMinLatEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mMinLat=0;
		}
		
		try{
			mMaxLat=Float.valueOf(mMaxLatEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mMaxLat=0;
		}
		
		try {
			mMinLon=Float.valueOf(mMinLonEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mMinLon=0;
		}
		
		try {
			mMaxLon=Float.valueOf(mMaxLonEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mMaxLon=0;
		}
	
		try {
			mMinZoom=Integer.valueOf(mMinZoomEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mMinZoom=1;
		}
		
		try {
			mMaxZoom=Integer.valueOf(mMaxZoomEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mMaxZoom=MAX_ZOOM_LEVEL;
		}
		
		try {
			mWidth=Integer.valueOf(mWidthEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mWidth=320;
		}
		
		try {
			mHeight=Integer.valueOf(mHeightEditText.getText().toString());
		}
		catch(NumberFormatException nfe) {			
			mHeight=480;
		}
		
	}
	
	private void updateControls() {
		
		mMinLatEditText.setText(String.valueOf(mMinLat));
		mMaxLatEditText.setText(String.valueOf(mMaxLat));
		
		mMinLonEditText.setText(String.valueOf(mMinLon));
		mMaxLonEditText.setText(String.valueOf(mMaxLon));
		
		mMinZoomEditText.setText(String.valueOf(mMinZoom));
		mMaxZoomEditText.setText(String.valueOf(mMaxZoom));
		
		mWidthEditText.setText(String.valueOf(mWidth));
		mHeightEditText.setText(String.valueOf(mHeight));
	}

	@Override
	public void onFocusChange(View v, boolean hasFocus) {
		
		readControls();
		
		checkValues();
		
		updateControls();				
	}
	
	@Override
    public void onBackPressed() {
		
		SharedPreferences.Editor prefsEditor=getSharedPreferences("OsmScoutBenchmark", MODE_PRIVATE).edit();
		
		prefsEditor.putFloat("MinLat", mMinLat);
		prefsEditor.putFloat("MaxLat", mMaxLat);
		
		prefsEditor.putFloat("MinLon", mMinLon);
		prefsEditor.putFloat("MaxLon", mMaxLon);
		
		prefsEditor.putInt("MinZoom", mMinZoom);
		prefsEditor.putInt("MaxZoom", mMaxZoom);
		
		prefsEditor.putInt("Width", mWidth);
		prefsEditor.putInt("Height", mHeight);
		
		prefsEditor.commit();
		
		finish();
	}

	@Override
	public void onStart(int totalCount) {
		
		mProgressDialog=new ProgressDialog(this);
		mProgressDialog.setTitle("Running Benchmark...");
		mProgressDialog.setMessage("Z=\nX=\nY=");
		mProgressDialog.setIndeterminate(false);
		mProgressDialog.setCancelable(true);
		mProgressDialog.setMax(totalCount);
		mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
		
		mProgressDialog.setOnCancelListener(this);
		
		mProgressDialog.show();
		
		mTotalCount=totalCount;
		
		for(int zoom=0; zoom<=MAX_ZOOM_LEVEL; zoom++) {
			
			mT1MinValues[zoom]=1000000;
			mT1MaxValues[zoom]=0;
			mT1TimeSum[zoom]=0;
			
			mT2MinValues[zoom]=1000000;
			mT2MaxValues[zoom]=0;
			mT2TimeSum[zoom]=0;
			
			mTileCount[zoom]=0;		
		}
	}
	
	@Override
	public void updateProgress(Integer... progress) {
		
		mProgressDialog.setProgress(progress[0].intValue());
		
		int zoom=progress[1];
		int x=progress[2];
		int y=progress[3];
		int t1=progress[4];
		int t2=progress[5];
		
		String message="Z="+zoom+"\nX="+x+"\nY="+y;
		
		mProgressDialog.setMessage(message);
		
		if (t1<mT1MinValues[zoom]) {
			mT1MinValues[zoom]=t1;
		}
		
		if (t1>mT1MaxValues[zoom]) {
			mT1MaxValues[zoom]=t1;
		}
		
		mT1TimeSum[zoom]+=t1;
		
		if (t2<mT2MinValues[zoom]) {
			mT2MinValues[zoom]=t2;
		}
		
		if (t2>mT2MaxValues[zoom]) {
			mT2MaxValues[zoom]=t2;
		}
		
		mT2TimeSum[zoom]+=t2;
		
		mTileCount[zoom]++;		
	}
	
	@Override
	public void onEnd() {
		
		mProgressDialog.dismiss();
		
		writeResultsToDisk();
	}
	
	private void writeResultsToDisk() {
		
		try {
			FileWriter outFile=new FileWriter(mMapPath+"/results.txt");
			
			outFile.write("# OsmScoutLib-Android Benchmark results\n\n");
			
			outFile.write("# Min Lon=  "+mMinLon+"\n");
			outFile.write("# Max Lon=  "+mMaxLon+"\n");
			outFile.write("# Min Lat=  "+mMinLat+"\n");
			outFile.write("# Max Lat=  "+mMaxLat+"\n");
			outFile.write("# Min Zoom= "+mMinZoom+"\n");
			outFile.write("# Max Zoom= "+mMaxZoom+"\n");
						
			for(int zoom=1; zoom<=MAX_ZOOM_LEVEL; zoom++) {
				
				if (mTileCount[zoom]==0)
					continue;
				
				outFile.write("\n# Zoom <"+zoom+"> \n");
				
				outFile.write("   TileCount="+mTileCount[zoom]+"\n");
				
				outFile.write("   GetObjects (Min/Avg/Max)= "+mT1MinValues[zoom]+" / "+
															 mT1TimeSum[zoom]/mTileCount[zoom]+" / "+
															 mT1MaxValues[zoom]+"\n");
				
				outFile.write("   DrawMap (Min/Avg/Max)= "+mT2MinValues[zoom]+" / "+
						 								   mT2TimeSum[zoom]/mTileCount[zoom]+" / "+
						 								   mT2MaxValues[zoom]+"\n");
			}
			
			outFile.write("\n#--- End of File ---\n");
			
			outFile.flush();
			
			outFile.close();
			
		} catch (IOException e) {
			
			return;
		}
		
		
	}

	@Override
	public void onCancel(DialogInterface dialog) {
		
		mTask.cancel(false);
	}
	
}
