#version 150 core

attribute vec2 position;
//attribute vec3 color;
//uniform mat4 Model;
//uniform mat4 View;
//uniform mat4 Projection;

void main() {
    gl_Position = vec4(position, 0.0, 1);
    //gl_Position = Model * View * Projection * vec4(position, 0.0, 1);
}