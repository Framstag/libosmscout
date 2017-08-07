#version 150 core

in vec2 Normal;
in vec3 Color;
in vec3 Barycentric;
in float RenderingMode;
out vec4 outColor;

void main() {
        float normalLength = length(Normal);
        vec3 d = fwidth(Barycentric);
        vec3 a3 = smoothstep(vec3(0.0), d*1.5, Barycentric);
        float r = min(min(a3.x, a3.y), a3.z);

        if(RenderingMode > 0)
            outColor = vec4(Color, 1.0 - normalLength);
        else{
            if(normalLength > 0.5)
                outColor = vec4(Color, r);
            else
                outColor = vec4(Color, 1.0);
        }
}