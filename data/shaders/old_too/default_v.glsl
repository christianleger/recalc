// Vertex Shader
#version 150

in vec3 InVertex ;      // attrib location 0
in vec3 InTexCoord0 ;   // attrib location 1
//in vec3 InNormal ;      // attrib location 2

uniform mat4 ProjectionModelviewMatrix ;

//out vec3 color ;
out vec3 tex_coord ;

void main()
{
    //color = gl_Color ;
    //gl_FrontColor = gl_Color ;
	gl_Position = ProjectionModelviewMatrix * vec4(InVertex, 1.0) ;
    tex_coord = vec3(InTexCoord0.xy, InTexCoord0.z) ;
}


// Attributes In 
//in vec4 InColor ;       // attrib location 3
// Uniforms In
// OUT
//out vec4 Position ;
//out vec4 Color ;
//    Color = InColor ;
//    tex_coord = vec3(InTexCoord0.xy,InTexCoord0.z+0.401) ;
// what fucking version should I be using? It seems unpossible and ridiculous to aim for a version supported across several completely 
// contemporaneous platforms. Shut uf the pucking shut up if you want to claim this is all not a problem. 
// Color = vec4( float(((uint(InColor)>>24)&uint(0xff000000))), float(((uint(InColor)>>16)&uint(0x00ff0000))), 
//               float(((uint(InColor)>>8)&uint(0x0000ff00))), float((uint(InColor)&uint(0x000000ff)))) ;
// Color = vec4( 1.0, 0.0, 0.0, 1.0 ) ; 

