#version 150 core

in vec2 position;        // Geographic coordinates of vertex
in vec4 color;           // Color of the area

out vec4 Color;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

vec2 PixelToGeo(in float x, in float y, in float latOffset);

vec2 GeoToPixel(in float posx, in float posy);

void main() {
    Color = color;
    vec2 result = GeoToPixel(position.x, position.y);

    gl_Position = Projection * View * Model * vec4(result.x, result.y, 0.0, 1.0);
}