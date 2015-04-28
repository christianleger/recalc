// GLSL Version 1.2 Fragment Shader: used to operate on older machines. 
// Example target hardware: a 2008 Intel GMA GPU. 
//
// This shader just paints a fragment with a texture value. 
//
#version 120

uniform sampler2D texatlas ; // Texture atlas
uniform float   atlasScale ;

varying vec2 texcoord ;
varying vec2 texoffset ;

void main()
{
    //gl_FragColor = texture2D(texatlas, vec2(texcoord.x*atlasScale, texcoord.y*atlasScale)) ;
    gl_FragColor = texture2D(texatlas, vec2(texcoord.x*atlasScale, texcoord.y*atlasScale)) ;
}

