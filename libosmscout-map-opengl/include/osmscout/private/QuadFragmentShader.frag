#version 150 core

//in vec3 Color;
in vec2 Texcoord;
in float TextureIndex;
in float NumOfTextures;
out vec4 outColor;

//uniform sampler2DArray tex;
uniform sampler2D tex;

void main() {
    outColor = texture(tex, Texcoord); //* vec4(1.0, 0.0, 0.0, 0.5);
    //outColor = texture(tex, vec3(Texcoord,0.0)); //* vec4(Color, 1.0);
    //outColor = texture2DArray(tex, vec3(Texcoord,TextureIndex));
   //float actual_layer = max(0, min(NumOfTextures​ - 1, floor(TextureIndex​ + 0.5)));
}