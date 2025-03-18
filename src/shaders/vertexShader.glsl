#version 430 core

uniform uint render_mode;
#define DEBUG 0
#define DIFFUSE 1

#define VECTOR3 vec3 
#define MATRIX4 mat4 




// Debug
#define MAX_DEBUG_LINES 10
#define MAX_DEBUG_AABB 10
#define MAX_POINT_LIGHTS 1

uniform VECTOR3 debugColours[MAX_DEBUG_LINES + MAX_DEBUG_AABB];
out vec4 diffuse_colour;
out VECTOR3 debug_colour;


uniform VECTOR3 point_lights_ws[MAX_POINT_LIGHTS];
uniform float radius;
uniform MATRIX4 view;
uniform MATRIX4 projection;




struct ParticleData
{
    VECTOR3 position;
    vec4 colour;
};

layout(std430, binding = 3) readonly buffer position_buffer
{
    ParticleData particle[];
};

out vec2 uv;
out VECTOR3 particle_pos_vs;





void main()
{
    vec4 pos = vec4(0.0, 0.0, 0.0, 1.0);
    int point_idx = gl_VertexID / 6;


    if(render_mode == DEBUG)
        pos =  vec4(point_lights_ws[point_idx], 1.0);


    else if(render_mode == DIFFUSE)
    {
        pos = vec4(particle[point_idx].position, 1.0);
        
        diffuse_colour = particle[point_idx].colour;

        particle_pos_vs = VECTOR3(view * pos);
    }


    const int indices[6] = {0, 2, 1, 2, 3, 1};

    const vec2 quad_uv[4] = {
                                vec2(1.0, -1.0), //  br
                                vec2(1.0, 1.0), //  tr
                                vec2(-1.0, -1.0), //  bl
                                vec2(-1.0, 1.0)  //  tl
                            };


    float rx = radius;
    float ry = radius;
    MATRIX4 view_to_world_transform = inverse(view);
    vec4 x = view_to_world_transform[0];
    vec4 y = view_to_world_transform[1];
    vec4 quad_pos[4] = {
                            vec4(pos + rx * x - ry * y), //  br
                            vec4(pos + rx * x + ry * y), //  tr
                            vec4(pos - rx * x - ry * y), //  bl
                            vec4(pos - rx * x + ry * y)  //  tl
                        };

    int quad_idx = indices[gl_VertexID % 6];
    gl_Position =  projection * view * quad_pos[quad_idx];
    uv = quad_uv[quad_idx];
}