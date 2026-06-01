#version 150 core

in vec2 Texcoord;
in vec4 Color;

uniform sampler2D tex;

out vec4 outColor;

void main() {
    float a = texture(tex, Texcoord).r;
    outColor = vec4(Color.rgb, Color.a * a);
}
