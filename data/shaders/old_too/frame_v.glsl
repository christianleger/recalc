// Vertex Shader
#version 150

in vec3 InVertex ;      // attrib location 0

uniform mat4 ProjectionModelviewMatrix ;
uniform vec4 colorX ;

out vec4 theColor ;

void main()
{
	gl_Position = ProjectionModelviewMatrix * vec4(InVertex, 1.0) ;
    theColor = colorX ;
}

