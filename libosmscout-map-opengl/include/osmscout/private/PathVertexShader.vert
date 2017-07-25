#version 150 core

in vec2 position;
in vec2 previous;
in vec2 next;
in vec3 color;
in float index;
in float thickness;
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
    float thickness_norm = ceil(thickness)/windowWidth;

    vec2 n = GeoToPixel(next.x, next.y);
    vec2 c = GeoToPixel(position.x, position.y);
    vec2 p = GeoToPixel(previous.x, previous.y);

    n = vec2(n.x, n.y);
    c = vec2(c.x, c.y);
    p = vec2(p.x, p.y);

    vec4 pos = Projection * View * Model * vec4(c.x, c.y, 0, 1);

    vec2 normal;
    vec2 result;
	if(index == 1.0){
	    float nx = (n.x - c.x);
        float ny = (n.y - c.y);
        normal = normalize(vec2(ny, -nx));
        result = normal;
    }
	else if(index == 2.0){
    	float nx = (n.x - c.x);
        float ny = (n.y - c.y);
        normal = normalize(vec2(-ny, nx));
        result = normal;
	}
	else if(index == 3.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(tangent.y, -tangent.x);
        result = miter;
	}
	else if(index == 4.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(-tangent.y, tangent.x);
        result = miter;
	}
	else if(index == 5.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(tangent.y, -tangent.x);
        result = miter;
	}
	else if(index == 6.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(-tangent.y, tangent.x);
        result = miter;
	}
	else if(index == 7.0){
	    float nx = (c.x - p.x);
        float ny = (c.y - p.y);
        normal = normalize(vec2(ny, -nx));
        result = normal;
	}
	else{
	    float nx = (c.x - p.x);
        float ny = (c.y - p.y);
        normal = normalize(vec2(-ny, nx));
        result = normal;
	}

    vec4 delta = vec4(result * thickness_norm, 0, 0);
    gl_Position = (pos + delta);
}