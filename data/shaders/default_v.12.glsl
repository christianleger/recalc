// 'old' Vertex Shader: old glsl version used for somewhat oldish hardware, 
// such as a 2008 Intel GMA GPU. If it ain't broke don't phase it out you know what I mean. 
#version 120

attribute vec3 InVertex ;      // attrib location 0
attribute vec3 InTexCoord0 ;   // attrib location 1

uniform mat4 ProjectionModelviewMatrix ;
//uniform int tex ;

varying vec2 texcoord ;
varying vec2 texoffset ;

void main()
{
	gl_Position = ProjectionModelviewMatrix * vec4(InVertex, 1.0) ;
    //  gl_TexCoord[0] = vec2(InTexCoord0.xy) ;
    //  tex_coord = vec3(InTexCoord0.xy, InTexCoord0.z) ;
    //  texcoord = vec2(InTexCoord0.xy) ;
    texcoord = InTexCoord0.xy ;

    int t = int(InTexCoord0.z) ;
//texoffset.x = float((t%8)*512) * 0.125 ;
//texoffset.y = (t - (t % 8)) * 32 ;

texoffset.x = 0 ;
texoffset.y = 0 ;

}
