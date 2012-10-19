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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class OsmScoutMapView extends View {

  private int mMapWidth=-1;
  private int mMapHeight=-1;

  private Bitmap mMapBitmap=null;
  private MapPainterCanvas mMapPainter=null;
  private OsmScoutMapEventListener mMapEventListener=null;
  
  private Paint mPaint=new Paint();

  public OsmScoutMapView(Context context) {
    super(context);

    init(context);
  }

  public OsmScoutMapView(Context context, AttributeSet attrs) {
    super(context, attrs);

    init(context);
  }

  public OsmScoutMapView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);

    init(context);
  }

  private void init(Context context) {

    mMapPainter=new MapPainterCanvas();
  }

  public void setMapEventListener(OsmScoutMapEventListener listener) {

    mMapEventListener=listener;
  }

  public void drawMap(StyleConfig styleConfig, MercatorProjection projection,
		  				MapParameter mapParameter, MapData mapData) {

    if (mMapBitmap==null)
      return;

    mMapPainter.drawMap(styleConfig, projection, mapParameter, mapData,
    					new Canvas(mMapBitmap));

    invalidate();
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {

    mMapWidth=w;
    mMapHeight=h;

    mMapBitmap=Bitmap.createBitmap(mMapWidth, mMapHeight, Bitmap.Config.ARGB_8888);

    if (mMapEventListener!=null) {

      mMapEventListener.onMapSizeChanged(mMapWidth, mMapHeight);
    }
  }

  @Override
  protected void onDraw(Canvas canvas) {

    if (mMapBitmap==null)
      return;

    canvas.drawBitmap(mMapBitmap, 0, 0, mPaint);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {

    boolean eventHandled=true;

    switch(event.getAction()) {

    case MotionEvent.ACTION_DOWN:

      if (mMapEventListener!=null) {

        int x=(int)event.getX();
        int y=(int)event.getY();

        mMapEventListener.onActionDown(x, y);
      }

      break;

    case MotionEvent.ACTION_MOVE:

      if (mMapEventListener!=null) {

        int x=(int)event.getX();
        int y=(int)event.getY();

        mMapEventListener.onActionMove(x, y);
      }

      break;

    case MotionEvent.ACTION_UP:
    case MotionEvent.ACTION_CANCEL:
      break;

    default:
      eventHandled=false;
      break;
    }

    return eventHandled;
  }
}

