#version 150 core

uniform float centerLat;
uniform float centerLon;
uniform float scaleGradtorad;
uniform uint useLinearInterpolation;
uniform float scaledLatDeriv;
uniform float latOffset;
uniform float scale;
uniform float angle;
uniform float angleNegSin;
uniform float angleNegCos;
uniform float windowWidth;
uniform float windowHeight;

uniform float PI = 3.1415926535897;
uniform float MaxLat = +85.0511;
uniform float MinLat = -85.0511;
uniform float Gradtorad = 1.745329251994330e-02; // 2*PI/360;

/**
 * Converts a geographic coordinate to screen pixel
 * see MercatorProjection::GeoToPixel
 */
vec2 GeoToPixel(in float lon, in float lat){
    float width = windowWidth;
    float height = windowHeight;
    // Calculations for Mercator projection

    // Screen coordinate relative to center of image
    float x=(lon-centerLon)*scaleGradtorad;
    float y;

    if (useLinearInterpolation==1u) {
        y=(lat-centerLat)*scaledLatDeriv;
    }
    else {
        // Mercator is defined just for latitude +-85.0511
        // For values outside this range is better to result projection border
        // than some invalid coordinate, like INFINITY
        float lat = min(max(lat, MinLat), MaxLat);
        y=(atanh(sin(lat*Gradtorad))-latOffset)*scale;
    }

    if (angle!=0.0) {
        float xn=x*angleNegCos-y*angleNegSin;
        float yn=x*angleNegSin+y*angleNegCos;

        x=xn;
        y=yn;
    }

    // Transform to OpenGL position
    float screenX = (x / (width/2)) * (width/height);
    float screenY = y / (height/2);

    vec2 result = vec2(screenX, screenY);
    return(result);
}

