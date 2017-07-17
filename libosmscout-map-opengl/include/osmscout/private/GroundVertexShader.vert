#version 150 core

in vec2 position;
in vec3 color;
out vec3 Color;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform float minLon;
uniform float minLat;
uniform float maxLon;
uniform float maxLat;
uniform float windowWidth;
uniform float windowHeight;
uniform float centerLat;
uniform float centerLon;

uniform float magnification;
uniform float dpi = 96.0;

//uniforms for Mercator projection
uniform float PI = 3.1415926535897;
uniform float R = 6378137.0;

vec2 PixelToGeo(in float x, in float y, in float latOffset)
{
    float tileDPI=96.0;
    float gradtorad=2*PI/360;
    float earthRadiusMeter=6378137.0;
    float earthExtentMeter=2*PI*earthRadiusMeter;
    float tileWidthZoom0Aquator=earthExtentMeter;
    float equatorTileWidth=tileWidthZoom0Aquator/magnification;
    float equatorTileResolution=equatorTileWidth/256.0;
    float equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/96;
    float groundWidthEquatorMeter=windowWidth*equatorCorrectedEquatorTileResolution;
    //float groundWidthVisibleMeter=groundWidthEquatorMeter*cos(gc.GetLat()*gradtorad);

    float scale=windowWidth/(2*PI*groundWidthEquatorMeter/earthExtentMeter);
    float scaleGradtorad=scale*gradtorad;

    x-=windowWidth/2;
    y=windowHeight/2-y;

    // Transform to absolute geo coordinate
    float lon=centerLon+x/scaleGradtorad;
    float lat=atan(sinh(y/scale+latOffset))/gradtorad;

    vec2 r = vec2(lon,lat);
    return (r);
}

vec2 GeoToPixel(in float posx, in float posy){
    float tileDPI=96.0;
    float gradtorad=2*PI/360;
    float earthRadiusMeter=6378137.0;
    float earthExtentMeter=2*PI*earthRadiusMeter;
    float tileWidthZoom0Aquator=earthExtentMeter;
    float equatorTileWidth=tileWidthZoom0Aquator/magnification;
    float equatorTileResolution=equatorTileWidth/256.0;
    float equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/96;
    float groundWidthEquatorMeter=windowWidth*equatorCorrectedEquatorTileResolution;
    float groundWidthVisibleMeter=groundWidthEquatorMeter*cos(posy*gradtorad);

    float latOffset=atanh(sin(centerLat*gradtorad));

    vec2 tl = PixelToGeo(0.0,0.0,latOffset);
    vec2 tr = PixelToGeo(windowWidth,0.0,latOffset);
    vec2 bl = PixelToGeo(0.0,windowHeight,latOffset);
    vec2 br = PixelToGeo(windowWidth,windowHeight,latOffset);

    float MaxLat = +85.0511;
    float MinLat = -85.0511;
    float MaxLon = +180.0;
    float MinLon = -180.0;

    float latMin=max(MinLat,min(min(tl.y,tr.y),min(bl.y,br.y)));
    float latMax=min(MaxLat,max(max(tl.y,tr.y),max(bl.y,br.y)));

    float lonMin=max(MinLon,min(min(tl.x,tr.x),min(bl.x,br.x)));
    float lonMax=min(MaxLon,max(max(tl.x,tr.x),max(bl.x,br.x)));

    float scale=windowWidth/(2*PI*groundWidthEquatorMeter/earthExtentMeter);
    float scaleGradtorad=scale*gradtorad;

    //float lonOffset=lonMin*scaleGradtorad;
    float latDeriv = 1.0 / sin( (2 * centerLat * gradtorad + PI) /  2);
    float scaledLatDeriv = latDeriv * gradtorad * scale;

    float x2=(posx-centerLon)*scaledLatDeriv;
    //float y2 = (posy-centerLat)*scaledLatDeriv;
    float y2=(atanh(sin(posy*gradtorad))-latOffset)*scale;

    y2=windowHeight/2-y2;
    x2 += windowWidth/2;

    float deg_rad = 180 / PI;
    float minLat_m = (log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    float height = abs(minLat_m - maxLat_m);
    float width = abs(minLon - maxLon);
    float x_width = width/height;
    float y_height = 1;

    float x3 = ((2*x_width)*(x2)/((windowWidth)))-x_width;
    float y3 = ((2*y_height)*(y2)/((windowHeight)))-y_height;

    //float height = abs(MinLat - MaxLat);
    //float width = abs(MinLon - MaxLon);
    //float x_width = windowWidth/windowHeight;
    //float y_height = 1;

    /*float minx = (minLon-centerLon)*scaledLatDeriv;
    float miny = (atanh(sin(minLat*gradtorad))-latOffset)*scale;
    float maxx = (maxLon-centerLon)*scaledLatDeriv;
    float maxy = (atanh(sin(maxLat*gradtorad))-latOffset)*scale;

    miny=windowHeight/2-miny;
    maxy=windowHeight/2-maxy;
    minx += windowWidth/2;
    maxx += windowWidth/2;

    float height = abs(miny - maxy);
    float width = abs(minx - maxx);
    float x_width = width/height;
    float y_height = 1;

    float x3 = ((2*x_width)*(x2 - (minx))/((maxx)-(minx)))-x_width;
    float y3 = ((2*y_height)*(y2 - (miny))/((maxy)-(miny)))-y_height;*/

    //float x3 = ((2*x_width)*(x2 - (bl.x))/((br.x)-(bl.x)))-x_width;
    //float y3 = ((2*y_height)*(y2 - (bl.y))/((tr.y)-(bl.y)))-y_height;

    vec2 v = vec2(x3, y3);
    return(v);
}


void main() {
    Color = color;
    //Mercator projection
    /*float deg_rad = 180 / PI;
    float y1 = log(tan((position.y / deg_rad) / 2 + PI / 4));
    float merc_y = y1 * deg_rad;

    float minLat_m = (log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    float height = abs(minLat_m - maxLat_m);
    float width = abs(minLon - maxLon);
    float x_width = width/height;
    float y_height = 1;

    float x = ((2*x_width)*(position.x - (minLon))/((maxLon)-(minLon)))-x_width;
    float y = ((2*y_height)*(merc_y - (minLat_m))/((maxLat_m)-(minLat_m)))-y_height;*/

    //float rx, ry;
    vec2 pos = GeoToPixel(position.x, position.y);

   gl_Position = Projection * View * Model * vec4(pos.x, -pos.y, 0.0, 1.0);
   // gl_Position = Projection * View * Model * vec4(pos.x, -pos.y, 0.0, 1.0);
    //gl_Position = Projection * View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = View * Modecl * vec4(x, y, 0.0, 1.0);
    //gl_Position = vec4(position, 0.0, 1.0);
}


/*#version 150 core

in vec2 position;
in vec3 color;
out vec3 Color;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform float minLon;
uniform float minLat;
uniform float maxLon;
uniform float maxLat;

//uniforms for Mercator projection
uniform float PI = 3.1415926535897;
uniform float R = 6378137.0;

void main() {
    Color = color;
    //Mercator projection
    float deg_rad = 180 / PI;
    float y1 = log(tan((position.y / deg_rad) / 2 + PI / 4));
    float merc_y = y1 * deg_rad;

    float minLat_m = (log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    float height = abs(minLat_m - maxLat_m);
    float width = abs(minLon - maxLon);
    float x_width = width/height;
    float y_height = 1;

    float x = ((2*x_width)*(position.x - (minLon))/((maxLon)-(minLon)))-x_width;
    float y = ((2*y_height)*(merc_y - (minLat_m))/((maxLat_m)-(minLat_m)))-y_height;

    gl_Position = Projection * View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position =  View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = vec4(position, 0.0, 1.0);
}*/
