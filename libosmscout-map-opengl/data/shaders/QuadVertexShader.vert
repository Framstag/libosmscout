#version 150 core

in vec2 position;        // Geographic coordinates of vertex
in float index;          // Type of the vertex. Necessary for creating the quad out of the coordinates.
in float textureStart;   // Where does its texture start in the texture atlas
in float textureWidth;   // Width of the texture of the quad
out vec2 Texcoord;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform float windowWidth;
uniform float windowHeight;
uniform float quadWidth;
uniform float textureWidthSum;

uniform float z;

vec2 GeoToPixel(in float posx, in float posy);

void main() {
    float width_norm = (ceil(quadWidth)/windowWidth);
    float height_norm = (ceil(quadWidth)/windowHeight);
    vec2 c = GeoToPixel(position.x, position.y);
    vec4 pos = Projection * View * Model * vec4(c.x, c.y, z, 1);

    float unit = 1/textureWidthSum;
    float i = (unit*textureStart) + (unit*textureWidth);
    float j = (unit*textureStart);

    gl_Position = pos;
	if(index == 1.0){
        gl_Position = pos;
        Texcoord = vec2(j, 1.0);
    }
	else if(index == 2.0){
	    gl_Position = vec4(pos.x + width_norm, pos.y, pos.z, pos.w);
	    Texcoord = vec2(i, 1.0);
	}
	else if(index == 3.0){
	    gl_Position = vec4(pos.x + width_norm, pos.y + height_norm, pos.z, pos.w);
	    Texcoord = vec2(i, 0.0);
	}
	else{
	    gl_Position = vec4(pos.x, pos.y + height_norm, pos.z, pos.w);
	    Texcoord = vec2(j, 0.0);
	}
}