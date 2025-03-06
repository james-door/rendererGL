#version 430 core

uniform uint render_mode;
#define LINE 0
#define FLAT 1
#define PHONG 2


out vec4 colour;

in vec3 debugColour;
in vec3 pos_ws;
in vec3 normal;


uniform vec3 camera_pos_ws;
in vec3 light_pos_ws; // only support single light for now
in mat4x4 model;


vec3 goochDiffuse(vec3 normal, vec3 lightDir, vec3 lightColour, vec3 diffuseColour)
{
    vec3 warm = vec3(0.3, 0.3, 0);
    vec3 cool = vec3(0, 0, 0.55);

    float alpha = 0.2;
    float beta = 0.8;
    
    vec3 k_cool = cool + alpha * diffuseColour;
    vec3 k_warm = warm + beta * diffuseColour;


    float t = 1.0 + dot(normal,lightDir) / 2.0;
    vec3 diffuse = t * k_warm + (1.0 - t) * k_cool;
    return diffuse;
}


// All vectors should be normalised
vec3 diffuseReflection(vec3 normal, vec3 lightDir, vec3 lightColour, vec3 diffuseColour, vec3 ambientColour)
{
    vec3 directColour = lightColour * max(dot(normal, lightDir), 0);
    return ((ambientColour + directColour) * diffuseColour);
}

// All vectors should be normalised
vec3 specularReflection(vec3 normal, vec3 lightDir, vec3 viewDir,float shininess, vec3 lightColour,vec3 specularColour)
{
    vec3 reflectedColour = lightColour * pow(max(dot(reflect(lightDir,normal),viewDir),0.0),shininess);

    return (reflectedColour * specularColour);
}


void main()
{
    if(render_mode == LINE)
        colour = vec4(debugColour, 1.0);
    if(render_mode == FLAT)
        colour = vec4(1.0,1.0,1.0,1.0);
    if(render_mode == PHONG)
    {
        vec3 lightColour = vec3(1.0, 1.0, 1.0);
        vec3 diffuseColour = vec3(1.0,0.0,0.0); // pre-multiply by pi
        // vec3 specularColour = vec3(1.0,1.0,1.0);
        // float shininess = 10.0;
        
        mat3x3 tempTransform = mat3x3(transpose(inverse(model)));

        vec3 n = normalize(tempTransform * normal);

        vec3 fragPos = pos_ws;
        vec3 lightDir = normalize(light_pos_ws - fragPos);
        vec3 viewDir = normalize(fragPos - camera_pos_ws);

        vec3 diffuse = goochDiffuse(n,lightDir,lightColour,diffuseColour);

        // vec3 specular = specularReflection(n,lightDir,viewDir,shininess,lightColour,specularColour);
        colour = vec4((diffuse), 1.0);
        
    }
}