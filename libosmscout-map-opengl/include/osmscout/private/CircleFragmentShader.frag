#version 150

in vec3 Color;
in vec2 Pos;
in vec2 WindowPos;
in vec2 WindowPos2;
in float Diameter;
out vec4 outColor;

void main() {
         float r = 0.0, delta = 0.0, alpha = 1.0;
         vec2 cxy = 2.0 * gl_PointCoord - 1.0;
         r = dot(cxy, cxy);
         if (r > 1.0) {
             discard;
         }

         delta = fwidth(r);
         alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);

         gl_FragColor = vec4(Color, alpha);
}
