#version 150 core

in vec2 position;
in vec4 color;
out vec4 Color;
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
uniform float dpi;

uniform float PI = 3.1415926535897;

vec2 PixelToGeo(in float x, in float y, in float latOffset)
{
    float tileDPI=96.0;
    float gradtorad=2*PI/360;
    float earthRadiusMeter=6378137.0;
    float earthExtentMeter=2*PI*earthRadiusMeter;
    float tileWidthZoom0Aquator=earthExtentMeter;
    float equatorTileWidth=tileWidthZoom0Aquator/magnification;
    float equatorTileResolution=equatorTileWidth/256.0;
    float equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/dpi;
    float groundWidthEquatorMeter=windowWidth*equatorCorrectedEquatorTileResolution;

    float scale=windowWidth/(2*PI*groundWidthEquatorMeter/earthExtentMeter);
    float scaleGradtorad=scale*gradtorad;

    x-=windowWidth/2;
    y=windowHeight/2-y;

    float lon=centerLon+x/scaleGradtorad;
    float lat=atan(sinh(y/scale+latOffset))/gradtorad;

    vec2 result = vec2(lon,lat);
    return (result);
}

vec2 GeoToPixel(in float posx, in float posy){
    float tileDPI=96.0;
    float gradtorad=2*PI/360;
    float earthRadiusMeter=6378137.0;
    float earthExtentMeter=2*PI*earthRadiusMeter;
    float tileWidthZoom0Aquator=earthExtentMeter;
    float equatorTileWidth=tileWidthZoom0Aquator/magnification;
    float equatorTileResolution=equatorTileWidth/256.0;
    float equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/dpi;
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

    float latDeriv = 1.0 / sin( (2 * centerLat * gradtorad + PI) /  2);
    float scaledLatDeriv = latDeriv * gradtorad * scale;

    float windowPosX=(posx-centerLon)*scaledLatDeriv;
    float windowPosY=(atanh(sin(posy*gradtorad))-latOffset)*scale;

    windowPosY=windowHeight/2-windowPosY;
    windowPosX += windowWidth/2;

    float MinX = (lonMin-centerLon)*scaledLatDeriv + windowWidth/2;
    float MinY = windowHeight/2 - (atanh(sin(latMin*gradtorad))-latOffset)*scale;
    float MaxX = (lonMax-centerLon)*scaledLatDeriv + windowWidth/2;
    float MaxY = windowHeight/2 - (atanh(sin(latMax*gradtorad))-latOffset)*scale;

    float newWidth = windowWidth/windowHeight;
    float newHeight = 1;

    float screenX = ((2*newWidth)*(windowPosX - (MinX))/((MaxX)-(MinX)))-newWidth;
    float screenY = ((2*newHeight)*(windowPosY - (MinY))/((MaxY)-(MinY)))-newHeight;

    vec2 result = vec2(screenX, screenY);
    return(result);
}

void main() {
    Color = color;
    vec2 result = GeoToPixel(position.x, position.y);

    gl_Position = Projection * View * Model * vec4(result.x, result.y, 0.0, 1.0);
}