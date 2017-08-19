#version 150 core

in vec2 Texcoord;
in vec4 Color;
out vec4 outColor;

uniform sampler2D tex;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(tex, Texcoord).r);
    outColor = Color * sampled;
}