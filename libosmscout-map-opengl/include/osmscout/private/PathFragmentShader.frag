#version 150 core

in vec2 Normal;
in vec3 Color;
out vec4 outColor;

void main() {
    float normalLength = length(Normal);
    if(normalLength < 0.5)
       outColor = vec4(Color, (1.0 - normalLength) + 0.2);
    else
       outColor = vec4(Color, 1.0 - normalLength);
}