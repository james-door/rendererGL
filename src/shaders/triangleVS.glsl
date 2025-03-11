#version 430 core

void main()
{
    vec3 triangle_ndc[] = {  
                            vec3( 0.5, -0.5, 0.0), //BR
                            vec3(-0.5, -0.5, 0.0), //BL
                            vec3( 0.0,  0.5, 0.0) //top
                         };
    gl_Position = vec4(triangle_ndc[gl_VertexID], 1.0);
}