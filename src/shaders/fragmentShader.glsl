#version 430 core

#define MAX_POINT_LIGHTS 1


#define VECTOR3 vec3 
#define MATRIX4 mat4 

uniform uint render_mode;
#define DEBUG 0
#define DIFFUSE 1


out vec4 colour;

in vec2 uv;
in VECTOR3 particle_pos_vs;
in vec4 diffuse_colour;


uniform VECTOR3 camera_pos_ws;
uniform float radius;


uniform VECTOR3 point_lights_ws[MAX_POINT_LIGHTS];

VECTOR3 diffuseColour(VECTOR3 normal, VECTOR3 light_dir, VECTOR3 colour)
{
    const VECTOR3 light_colour = VECTOR3(1.0, 1.0, 1.0);
    float t = max(dot(normal, light_dir), 0.0);
    return light_colour * colour * t;
}



void main()
{
    // colour =  vec4(1.0,1.0,1.0,1.0);
    // return;
    float length_squared = dot(uv,uv);

    if(length_squared > 1.0)
        discard;


    if(render_mode == DEBUG)
        colour = vec4(1.0,1.0,1.0, 1.0);
        
    else if(render_mode == DIFFUSE)
    {
        const VECTOR3 particle_colour = VECTOR3(1.0, 0.0, 0.0);

        //asume one light 
        // VECTOR3 light_pos_vs = VECTOR3(view * vec4(point_lights_ws[0], 1.0));
        VECTOR3 light_pos_vs = point_lights_ws[0];


        VECTOR3 normal_with_z =  VECTOR3(uv, -sqrt(1.0 - length_squared));

        VECTOR3 frag_pos_vs = radius * normal_with_z + particle_pos_vs;
        
        VECTOR3 light_dir = normalize(light_pos_vs - frag_pos_vs);


        VECTOR3 diffuse_rgb = VECTOR3(diffuse_colour);
        float diffuse_alpha = diffuse_colour.w;

        VECTOR3 diffuse = diffuseColour(normal_with_z, light_dir, diffuse_rgb);
        
        colour = vec4(diffuse, diffuse_alpha);

    }
}
