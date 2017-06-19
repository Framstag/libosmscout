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

    float x = (2*(position.x - (minLon))/((maxLon)-(minLon)))-1;
    float y = (2*(merc_y - (minLat_m))/((maxLat_m)-(minLat_m)))-1;
    gl_Position = Projection * View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = View * Model * vec4(x, y, 0.0, 1.0);
    //gl_Position = vec4(position, 0.0, 1.0);
}
