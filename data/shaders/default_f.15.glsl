// Fragment Shader
#version 150

in vec3 tex_coord ;

uniform sampler2DArray tex ;

out vec4 FragColor;
 
void main()
{
    vec3 tc = tex_coord ;
    FragColor = texture(tex, tc) ;


//    tc.x = gl_FrontColor.x + tc.x ;
//    tc.y = gl_FrontColor.x + tc.x ;
//    tc.x = gl_FrontColor.x + tc.x ;

//    vec4 frag = texture(tex, tc) ;
    //FragColor = frag + gl_FrontColor ;
// THIS BREAKS:  FragColor = gl_FrontColor ;
}

// From vertex shader
//in vec4 Color ;
// From application
//uniform sampler3D tex ;
//#extension GL_EXT_texture_array : enable
//tc.z = 1 ;
//FragColor = texture2DArray(tex, tex_coord.stp) ;
// How do we apply a color bias? 
// FragColor = texture(tex, tex_coord)*Color ;
