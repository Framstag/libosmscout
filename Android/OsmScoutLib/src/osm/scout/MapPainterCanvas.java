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

package osm.scout;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;

public class MapPainterCanvas {
	
	Canvas mCanvas=null;
	Paint mPaint=null;
	
	public MapPainterCanvas() {
		jniConstructor();
		
		mPaint=new Paint();
		
		mPaint.setAntiAlias(true);
	}
	
	protected void finalize() throws Throwable {		
		try {			
			jniDestructor();
		}
		finally {			
			super.finalize();
		}
	}
	
	public boolean drawMap(StyleConfig styleConfig, Projection projection,
			MapData mapData, Canvas canvas) {
		
		mCanvas=canvas;
		
		return jniDrawMap();
	}
	
	public void drawPath(int color, float width, float[] x, float[] y) {
		
		Path path=new Path();
		
		path.moveTo(x[0], y[0]);
		
		int numPoints=x.length;
		
		for(int i=0; i<numPoints; i++) {
			
			path.lineTo(x[i], y[i]);
		}
		
		mPaint.setColor(color);
		mPaint.setStyle(Paint.Style.STROKE);
		mPaint.setStrokeWidth(width);
		
		mCanvas.drawPath(path, mPaint);
	}
	
	public void drawArea(int color, float[] x, float[] y) {
		
		Path areaPath=new Path();
		
		areaPath.moveTo(x[0], y[0]);
		
		int numPoints=x.length;
		
		for(int i=0; i<numPoints; i++) {
			
			areaPath.lineTo(x[i], y[i]);
		}
		
		areaPath.close();
		
		mPaint.setColor(color);
		mPaint.setStyle(Paint.Style.FILL);
		
		mCanvas.drawPath(areaPath, mPaint);
	}
	
	public void drawArea(int color, float x, float y, float width, float height) {
		
		RectF rect=new RectF(x, y, width, height);
		
		mPaint.setColor(color);
		mPaint.setStyle(Paint.Style.FILL_AND_STROKE);
		
		mCanvas.drawRect(rect, mPaint);
	}
	
	// Private native methods
	
	private native void jniConstructor();
	private native void jniDestructor();
	private native boolean jniDrawMap();
}
