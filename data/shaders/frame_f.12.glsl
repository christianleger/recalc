// Fragment Shader
#version 120

//(from glsl 1.5) in vec4 theColor ;
varying vec4 theColor ;

//(from glsl 1.5) out vec4 FragColor;
 
void main()
{
    //FragColor = theColor ;
    gl_FragColor = theColor ;
}


