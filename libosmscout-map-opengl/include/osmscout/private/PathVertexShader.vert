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

    float x = (2*(position.x - (minLon))/((maxLon)-(minLon)))-1;
    float y = (2*(merc_y - (minLat_m))/((maxLat_m)-(minLat_m)))-1;
    float x_next = (2*(next.x - (minLon))/((maxLon)-(minLon)))-1;
    float y_next = (2*(merc_y_next - (minLat_m))/((maxLat_m)-(minLat_m)))-1;
    float x_prev = (2*(previous.x - (minLon))/((maxLon)-(minLon)))-1;
    float y_prev = (2*(merc_y_prev - (minLat_m))/((maxLat_m)-(minLat_m)))-1;
    float thickness_norm = thickness/((screenWidth/thickness)/2);

    vec2 n = vec2(x_next, y_next);
    vec2 c = vec2(x, y);
    vec2 p = vec2(x_prev, y_prev);

    vec4 pos = Projection * View * Model * vec4(x, y, 0, 1);

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
        float nx = (x - x_prev);
        float ny = (y - y_prev);
        normal = vec2(-ny, nx);

        float nx2 = (x_next - x);
        float ny2 = (y_next - y);
        vec2 normal2 = vec2(-ny2, nx2);

        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(-tangent.y, tangent.x);
        result = miter;
	}
	else if(index == 5.0){
	    float nx = (x_next - x);
        float ny = (y_next - y);
        normal = vec2(ny, -nx);

        float nx2 = (x - x_prev);
        float ny2 = (y - y_prev);
        vec2 normal2 = vec2(ny2, -nx2);

        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(tangent.y, -tangent.x);
        result = miter;
	}
	else if(index == 6.0){
		float nx = (x_next - x);
         float ny = (y_next - y);
	    normal = vec2(-ny, nx);

        float nx2 = (x - x_prev);
        float ny2 = (y - y_prev);
        vec2 normal2 = vec2(-ny2, nx2);

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
}