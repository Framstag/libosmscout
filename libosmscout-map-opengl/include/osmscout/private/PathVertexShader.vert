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

    vec2 v = vec2(x2, y2);
    return(v);
}


void main() {
    Color = color;
    float thickness_norm = thickness/((windowWidth/thickness)/2);

    vec2 n = GeoToPixel(next.x, next.y);
    vec2 c = GeoToPixel(position.x, position.y);
    vec2 p = GeoToPixel(previous.x, previous.y);

    n = vec2(n.x, -n.y);
    c = vec2(c.x, -c.y);
    p = vec2(p.x, -p.y);

    vec4 pos = Projection * View * Model * vec4(c.x, c.y, 0, 1);
    //vec4 pos = View * Model * vec4(x, y, 0, 1);

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
    //float rx, ry;

    //float x3 = ((2*x_width)*(x2)/((800)))-x_width;
    //float y3 = ((2*y_height)*(y2)/((600)))-y_height;

//gl_Position = Projection * View * Model * vec4(x, y, 0.0, 1.0);
//Color = vec3(pos.x, pos.y, 0);
//Color = vec3(0.92, 0.0, 0.0);
    //gl_Position = Projection * View * Model * vec4(pos.x, -pos.y, 0.0, 1.0);
    //gl_Position = Projection * View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = View * Modecl * vec4(x, y, 0.0, 1.0);
    //gl_Position = vec4(position, 0.0, 1.0);
}

/*#version 150 core
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
uniform float screenHeight;
uniform float screenWidth;

//uniforms for Mercator projection
uniform float PI = 3.1415926535897;
uniform float R = 6378137.0;

void main() {
    Color = color;
    float deg_rad = 180 / PI;
    float y1 = log(tan((position.y / deg_rad) / 2 + PI / 4));
    float y1_next = log(tan((next.y / deg_rad) / 2 + PI / 4));
    float y1_prev = log(tan((previous.y / deg_rad) / 2 + PI / 4));
    float merc_y = y1 * deg_rad;
    float merc_y_next = y1_next * deg_rad;
    float merc_y_prev = y1_prev * deg_rad;

    float minLat_m = (log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

     float height = abs(minLat_m - maxLat_m);
     float width = abs(minLon - maxLon);
     float x_width = width/height;
     float y_height = 1;

     float x = ((2*x_width)*(position.x - (minLon))/((maxLon)-(minLon)))-x_width;
     float y = ((2*y_height)*(merc_y - (minLat_m))/((maxLat_m)-(minLat_m)))-y_height;
     float x_next = ((2*x_width)*(next.x - (minLon))/((maxLon)-(minLon)))-x_width;
     float y_next = ((2*y_height)*(merc_y_next - (minLat_m))/((maxLat_m)-(minLat_m)))-y_height;
     float x_prev = ((2*x_width)*(previous.x - (minLon))/((maxLon)-(minLon)))-x_width;
     float y_prev = ((2*y_height)*(merc_y_prev - (minLat_m))/((maxLat_m)-(minLat_m)))-y_height;
     float thickness_norm = thickness/((screenWidth/thickness)/2);

    vec2 n = vec2(x_next, y_next);
    vec2 c = vec2(x, y);
    vec2 p = vec2(x_prev, y_prev);

    vec4 pos = Projection * View * Model * vec4(x, y, 0, 1);
    //vec4 pos = View * Model * vec4(x, y, 0, 1);

    vec2 normal;
    vec2 result;
	if(index == 1.0){
	    float nx = (x_next - x);
        float ny = (y_next - y);
        normal = normalize(vec2(ny, -nx));
        result = normal;
    }
	else if(index == 2.0){
    	float nx = (x_next - x);
        float ny = (y_next - y);
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
	    float nx = (x - x_prev);
        float ny = (y - y_prev);
        normal = normalize(vec2(ny, -nx));
        result = normal;
	}
	else{
	    float nx = (x - x_prev);
        float ny = (y - y_prev);
        normal = normalize(vec2(-ny, nx));
        result = normal;
	}

    vec4 delta = vec4(result * thickness_norm, 0, 0);
    gl_Position = (pos + delta);
}*/