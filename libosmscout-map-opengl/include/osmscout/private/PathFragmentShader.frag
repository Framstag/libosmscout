#version 150 core

in vec2 Normal;
in vec4 Color;
in vec3 Barycentric;
in float RenderingMode;
out vec4 outColor;

void main() {
        float normalLength = length(Normal);
        vec3 d = fwidth(Barycentric);
        vec3 a3 = smoothstep(vec3(0.0), d*1.5, Barycentric);
        float r = min(min(a3.x, a3.y), a3.z);

        if(RenderingMode > 0){
            outColor = vec4(Color.rgb, (1.0 - normalLength)*Color.a);
        }
        else{
            if(normalLength > 0.5){
              outColor = vec4(Color.rgb, r*Color.a);
            }
            else{
                outColor = vec4(Color.rgb, Color.a);
            }
        }
}