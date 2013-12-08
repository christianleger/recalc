// Vertex Shader
#version 120

//(from glsl 1.5) in vec3 InVertex ;      // attrib location 0
attribute vec3 InVertex ;      // attrib location 0

uniform mat4 ProjectionModelviewMatrix ;
uniform vec4 colorX ;

//(from glsl 1.5) out vec4 theColor ;
varying vec4 theColor ;

void main()
{
	gl_Position = ProjectionModelviewMatrix * vec4(InVertex, 1.0) ;
    theColor = colorX ;
}

