#version 430 core
layout (location = 0) in vec3 posWS;
layout (location = 1) in vec3 in_normal;

uniform uint render_mode;
#define LINE 0
#define FLAT 1
#define PHONG 2

uniform mat4x4 mvp;


// Debug
#define MAX_DEBUG_LINES 10
#define MAX_DEBUG_AABB 10
#define MAX_POINT_LIGHTS 1

uniform vec3 debugColours[MAX_DEBUG_LINES + MAX_DEBUG_AABB];
out vec3 debugColour;

// Paritcles
uniform float particle_scale;
out vec3 pos_ws;
out vec3 normal;
out mat4x4 model;



struct ParticleData
{
    vec3 position;
};

layout(std430, binding = 3) readonly buffer position_buffer
{
    vec3 point_light_pos[MAX_POINT_LIGHTS];
    ParticleData particle[];
};

out vec3 light_pos_ws; // only support single light for now

mat4 calculateModel(vec3 pos)
{
    float scale= particle_scale;
    model = mat4(
        scale,   0.0,     0.0,     0.0,
        0.0,     scale,   0.0,     0.0,
        0.0,     0.0,     scale,   0.0,
        pos.x,   pos.y,   pos.z,   1.0
    );
    return model;
}

void main()
{   
  
    if(render_mode == PHONG)
    {

        mat4 model = calculateModel(particle[gl_InstanceID].position);
        gl_Position = mvp * model * vec4(posWS, 1.0);
        pos_ws = vec3(model * vec4(posWS, 1.0));
        normal = in_normal;
        light_pos_ws = point_light_pos[0];
    }
    if(render_mode == FLAT)
    {
        mat4 model = calculateModel(point_light_pos[gl_InstanceID]);
        gl_Position = mvp * model * vec4(posWS, 1.0);
        debugColour = vec3(1.0f, 1.0f, 1.0f);
    }
    if(render_mode == LINE)
    {
        gl_Position = mvp * vec4(posWS,1.0);
        const int nVertsPerLine = 2;
        const int nVertsPerAABB = 24;
        int index = (gl_VertexID < MAX_DEBUG_LINES * nVertsPerLine)
            ? gl_VertexID / nVertsPerLine
            : MAX_DEBUG_LINES + (gl_VertexID - MAX_DEBUG_LINES * nVertsPerLine) / nVertsPerAABB;
        debugColour = debugColours[index];
    }

}