#version 410 core
layout (location = 0) in vec2 pos_ndc;

void main()
{     
    gl_Position = vec4(pos_ndc,0.0,1.0);
}
