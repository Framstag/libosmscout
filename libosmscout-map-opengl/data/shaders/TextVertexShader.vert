#version 150 core

in vec2 position;           // Geographic coordinates of vertex
in vec4 color;              // Color of the text
in float index;             // Type of the vertex. Necessary for creating the quad out of the coordinates.
in float textureStart;      // Where does its texture start in the texture atlas
in float textureWidth;      // Width of the texture of the quad in pixel
in float positionOffset;    // The text is rendered character by character, so if it is not the first character in the
                            // label, it needs to be offsetted by the sum width of the previous characters
in float startOffset;       // If there's an icon before the text it needs an offset in the x position
out vec2 Texcoord;
out vec4 Color;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform float windowWidth;
uniform float windowHeight;
uniform float textureHeight;
uniform float textureWidthSum;

uniform float z;

vec2 PixelToGeo(in float x, in float y, in float latOffset);

vec2 GeoToPixel(in float posx, in float posy);

void main() {
    Color = color;

    float width_norm = (ceil(textureWidth)/windowWidth);
    float height_norm = (ceil(textureHeight)/windowHeight);

    float offset = (ceil(positionOffset)/windowWidth) + (ceil(startOffset)/windowWidth);

    vec2 c = GeoToPixel(position.x, position.y);
    vec4 pos = Projection * View * Model * vec4(c.x, c.y, z, 1);

    float unit = 1/textureWidthSum;
    float i = (unit*textureStart) + (unit*textureWidth);
    float j = (unit*textureStart);

    gl_Position = pos;
	if(index == 1.0){
        gl_Position = vec4(pos.x + offset, pos.y, pos.z, pos.w);
        Texcoord = vec2(j, 1.0);
    }
	else if(index == 2.0){
	    gl_Position = vec4(pos.x + width_norm + offset, pos.y, pos.z, pos.w);
	    Texcoord = vec2(i, 1.0);
	}
	else if(index == 3.0){
	    gl_Position = vec4(pos.x + width_norm + offset, pos.y + height_norm, pos.z, pos.w);
	    Texcoord = vec2(i, 0.0);
	}
	else{
	    gl_Position = vec4(pos.x + offset, pos.y + height_norm, pos.z, pos.w);
	    Texcoord = vec2(j, 0.0);
	}

}