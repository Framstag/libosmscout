#version 150 core

in vec2 position;        // Geographic coordinates of vertex
in vec2 previous;        // Geographic coordinates of previous vertex
in vec2 next;            // Geographic coordinates of next vertex
in vec4 color;           // Color of the way
in vec4 gapcolor;        // Color of the gap if it is a dashed line
in float index;          // Type of the vertex. It is necessary for creating the quad out of the coordinates, and rendering
                         // the joints correctly.
                         // It represents which vertex is it in the quad, and is it the first/last vertex
                         // or is it one of the middle ones.
                         //
                         //  1 ----------- 3 5 ----------- 3 5 ----------- 7
                         //  |       _____/| |       _____/| |       _____/|
                         //  |  ____/      | |  ____/      | |  ____/      |
                         //  2 /__________ 4 6 /---------- 4 6 /---------- 8
                         //
in float thickness;      // Thickness of way in pixel
in float border;         // Decides whether it is a border or not
in vec3 barycentric;     // Barycentric coordinates a vertex. Necessary for anti-aliasing
in float z;              // Z coordinate
in float dashsize;       // Length of one dash if it is a dashed line
in float length;         // Full length of way in pixel

out vec2 Normal;
out vec4 Color;
out vec4 GapColor;
out vec3 Barycentric;
out float RenderingMode;
out float Dashed;
flat out float DashSize;
out float Dash;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform float windowWidth;

vec2 GeoToPixel(in float posx, in float posy);


void main() {
    Color = color;
    GapColor = gapcolor;
    Barycentric = barycentric;

    //+0.001 so lines dont disappear because of anti-aliasing
    //float thickness_norm = (ceil(thickness)/windowWidth) + 0.001;
    float thickness_norm;
    //Lines that are really thin, needs other type of rendering
    if(thickness < 2){
        RenderingMode = 1;
        thickness_norm = (ceil(thickness)/windowWidth) + 0.001;
    }
    else{
        RenderingMode = -1;
        thickness_norm = (ceil(thickness)/windowWidth);
        if(border != 0)
            thickness_norm += thickness_norm/10;
    }

    //Mercator projection, convertion to screen coordinates
    vec2 n = GeoToPixel(next.x, next.y);
    vec2 c = GeoToPixel(position.x, position.y);
    vec2 p = GeoToPixel(previous.x, previous.y);

    n = vec2(n.x, n.y);
    c = vec2(c.x, c.y);
    p = vec2(p.x, p.y);

    vec4 pos = Projection * View * Model * vec4(c.x, c.y, z, 1);

    //Calculations for dashed lines
    float dist = length / windowWidth;
    float dash = dashsize/windowWidth;
    float normalized_dash = (dash)/(dist);
    Dashed = (dashsize == 0) ? -1 : 1;
    DashSize = normalized_dash;

    vec2 normal;
    vec2 result;
	if(index == 1.0){
	    float nx = (n.x - c.x);
        float ny = (n.y - c.y);
        normal = normalize(vec2(ny, -nx));
        result = normal;
        Normal = normal;
        Dash = 0.0;
    }
	else if(index == 2.0){
    	float nx = (n.x - c.x);
        float ny = (n.y - c.y);
        normal = normalize(vec2(-ny, nx));
        result = normal;
        Normal = normal;
        Dash = 0.0;
	}
	else if(index == 3.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(tangent.y, -tangent.x);
        result = miter;
        Normal = miter;
        Dash = 1.0;
	}
	else if(index == 4.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(-tangent.y, tangent.x);
        result = miter;
        Normal = miter;
        Dash = 1.0;
	}
	else if(index == 5.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(tangent.y, -tangent.x);
        result = miter;
        Normal = miter;
        Dash = 0.0;
	}
	else if(index == 6.0){
        vec2 tangent = normalize(normalize(n-c) + normalize(c-p));
        vec2 miter = vec2(-tangent.y, tangent.x);
        result = miter;
        Normal = miter;
        Dash = 0.0;
	}
	else if(index == 7.0){
	    float nx = (c.x - p.x);
        float ny = (c.y - p.y);
        normal = normalize(vec2(ny, -nx));
        result = normal;
        Normal = normal;
        Dash = 1.0;
	}
	else{
	    float nx = (c.x - p.x);
        float ny = (c.y - p.y);
        normal = normalize(vec2(-ny, nx));
        result = normal;
        Normal = normal;
        Dash = 1.0;
	}

    vec4 delta = vec4(result * thickness_norm, 0, 0);
    gl_Position = (pos + delta);
}