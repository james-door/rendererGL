#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <span>
#include <cassert>
#include <charconv>
#include <array>
#include <algorithm>


#include "external/glad/glad.h"

#include "defintions.h"
#include "arena.h"
#include "utility.h"
#include "defintions.h"
#include "glmath.h"






i64 compileShader(std::string_view shader_blob, GLenum shaderType)
{
    // RENDERER_ASSERT(glXGetCurrentContext() != nullptr, "Called on thread without a valid context.");

    u32 shaderObj = glCreateShader(shaderType);
    i32 shaderCompiled;
    const char* shader_source = shader_blob.data();
    GLint shader_length = static_cast<GLint>(shader_blob.length());
    glShaderSource(shaderObj,1,&shader_source,&shader_length);
    glCompileShader(shaderObj);
    glGetShaderiv(shaderObj,GL_COMPILE_STATUS,&shaderCompiled);
    
    
    if(!shaderCompiled)
    {
        i32 messageLength;
        glGetShaderiv(shaderObj,GL_INFO_LOG_LENGTH,&messageLength);
        if(messageLength == 0)
            return -1;
        
        char *errorMsg = new char[messageLength];
        glGetShaderInfoLog(shaderObj,messageLength,NULL,errorMsg);

        RENDERER_LOG(errorMsg);
        delete[] errorMsg;
        return -1;
    }
    return shaderObj;
}



struct VertexPosNormal
{
    glmath::Vec3 pos;
    glmath::Vec3 normal;
};



struct ModelVertexData
{
    i32 vertex_offset;
    i32 index_offset;
    u32 n_verts;
    u32 n_indices;
};

struct ModelMetaData
{
    std::string_view blob;
    u32 nVerts;
    u32 nTriangles;
    u32 modelDataOffset;
};

void getPlyMetaData(ModelMetaData &metaData)
{
    constexpr char vertexNumberPreface[] = "vertex ";
    constexpr u8 vertexNumberPrefaceLength = 7;
    constexpr char faceNumberPreface[] = "face ";
    constexpr u8 faceNumberPrefaceLength = 5;
    constexpr char modelDataPreface[] = "end_header\n";
    constexpr u8 modelDataPrefaceLength = 11;

    u32 vertexNumberOffset =static_cast<u32>(metaData.blob.find(vertexNumberPreface) + vertexNumberPrefaceLength);

    u32 vertexNumberEnd = vertexNumberOffset;
    while(metaData.blob[vertexNumberEnd] != '\n') ++vertexNumberEnd;

    u32 faceNumberOffset = static_cast<u32>(metaData.blob.find(faceNumberPreface) + faceNumberPrefaceLength);
    u32 faceNumberEnd = faceNumberOffset;
    while(metaData.blob[faceNumberEnd] != '\n') ++faceNumberEnd;

    metaData.nVerts = stringToU32(&metaData.blob[vertexNumberOffset],vertexNumberEnd - vertexNumberOffset);
    metaData.nTriangles = stringToU32(&metaData.blob[faceNumberOffset], faceNumberEnd - faceNumberOffset);
    metaData.modelDataOffset = static_cast<u32>(metaData.blob.find(modelDataPreface) + modelDataPrefaceLength);
}


void loadIndices(SubArena &indexDataArena, const ModelMetaData& modelInfo, i64 offset)
{
    i32* indices = indexDataArena.arenaPush<i32>(modelInfo.nTriangles * 3);
    // Assume that all faces are triangles
    for(u32 i = 0; i < modelInfo.nTriangles; ++i)
    {
        // replace reinterpret_cast with memcpy
        u8 face_number = *reinterpret_cast<const u8*>(&modelInfo.blob[offset]); 
        offset += 1; // skip number of vertices in the face
        RENDERER_ASSERT(face_number == 3, "Mesh is not all triangles.");

        indices[i * 3] = *reinterpret_cast<const u32*>(&modelInfo.blob[offset]);
        offset += 4;
        indices[i * 3 + 1] = *reinterpret_cast<const u32*>(&modelInfo.blob[offset]);
        offset += 4;
        indices[i * 3 + 2] = *reinterpret_cast<const u32*>(&modelInfo.blob[offset]);
        offset += 4;
    }
}

void loadPlyModelPos(SubArena& vertexDataArena, SubArena& indexDataArena,ModelMetaData modelInfo)
{
    i64 offset = modelInfo.modelDataOffset;
    VertexPosNormal* vertices_pos_normal = vertexDataArena.arenaPush<VertexPosNormal>(modelInfo.nVerts);
    for(u32 i = 0; i < modelInfo.nVerts; ++i)
    {
        f32 x = *reinterpret_cast<const f32*>(&modelInfo.blob[offset]);
        offset += 4;
        f32 y = *reinterpret_cast<const f32*>(&modelInfo.blob[offset]);
        offset += 4;
        f32 z = *reinterpret_cast<const f32*>(&modelInfo.blob[offset]);
        offset += 4;

        f32 nx = *reinterpret_cast<const f32*>(&modelInfo.blob[offset]);
        offset += 4;
        f32 ny = *reinterpret_cast<const f32*>(&modelInfo.blob[offset]);
        offset += 4;
        f32 nz = *reinterpret_cast<const f32*>(&modelInfo.blob[offset]);
        offset += 4;
        vertices_pos_normal[i] = {
                        glmath::Vec3(x,y,z),
                        glmath::Vec3(nx,ny,nz)
                    };
    }
    loadIndices(indexDataArena,modelInfo,offset);
}


struct Line
{
    glmath::Vec3 points[2];
    Line(glmath::Vec3 p1, glmath::Vec3 p2)
    {
        points[0] = p1;
        points[1] = p2;
    }
    Line()
    {
        points[0] = {0.0,0.0,0.0};
        points[1] = {0.0,0.0,0.0};
    }

};
struct DebugAABB
{
    glmath::Vec3 verts[24];
   

    DebugAABB(glmath::Vec3 bl, glmath::Vec3 tr)
    {
        f32 width  = tr.x - bl.x;
        f32 height = tr.y - bl.y;
        f32 depth  = tr.z - bl.z;

        // bottom
        verts[0] = {bl.x, bl.y , bl.z};
        verts[1] = {bl.x + width, bl.y, bl.z};

        verts[2] = {bl.x + width, bl.y, bl.z};
        verts[3] = {bl.x + width, bl.y, bl.z + depth};

        verts[4] = {bl.x + width, bl.y, bl.z + depth};
        verts[5] = {bl.x, bl.y, bl.z + depth};

        verts[6] = {bl.x, bl.y , bl.z};
        verts[7] = {bl.x, bl.y, bl.z + depth};

        //top 
        verts[8] = {bl.x, bl.y + height , bl.z} ;
        verts[9] = {bl.x + width, bl.y + height, bl.z};

        verts[10] = {bl.x + width, bl.y + height, bl.z};
        verts[11] = {bl.x + width, bl.y + height, bl.z + depth};

        verts[12] = {bl.x + width, bl.y + height, bl.z + depth};
        verts[13] = {bl.x, bl.y + height, bl.z + depth};

        verts[14] = {bl.x, bl.y + height, bl.z} ;
        verts[15] = {bl.x, bl.y + height, bl.z + depth};

        //pillars
        verts[16] = {bl.x, bl.y , bl.z};
        verts[17] = {bl.x, bl.y + height, bl.z};

        verts[18] = {bl.x + width, bl.y , bl.z};
        verts[19] = {bl.x + width, bl.y + height, bl.z};

        verts[20] = {bl.x + width, bl.y, bl.z + depth};
        verts[21] = {bl.x + width, bl.y + height, bl.z + depth};

        verts[22] = {bl.x, bl.y, bl.z + depth};
        verts[23] = {bl.x, bl.y + height, bl.z + depth};
    }
    DebugAABB()
    {
        memset(verts,0,sizeof(verts));
    }

};

constexpr i32 MAX_DEBUG_LINES = 10;
constexpr i32 MAX_DEBUG_AABB = 10;
constexpr i32 MAX_POINT_LIGHTS = 1;


// use vec4 given that in std340/std130 vec3 has an alignment of 16 bytes
struct ParticleData
{
    glmath::Vec4 position;
    glmath::Vec4 colour;
};

struct Renderer
{
    SubArena debug_render_data; 
    std::span<DebugAABB> debug_aabb;
    std::span<Line> debug_lines;
    std::span<glmath::Vec3> colour;

    // SubArena dynamic_render_data; 
    // std::span<ParticleData> particle_data;
    // std::span<glmath::Vec4> light_pos;
    std::vector<ParticleData> particle_data;
    std::array<glmath::Vec3, MAX_POINT_LIGHTS> light_pos;

    i64 dynamic_sso_capacity;
    u32 dynamic_sso;

    u32 debug_vao;
    u32 dummy_vao; 

    i32 debug_colours_uniform;

    i32 shader_program;

    i32 render_mode_uniform;
    i32 projection_uniform;
    i32 view_uniform;
    i32 point_light_uniform;

    i32 particle_radius_uniform;
};




extern char _binary_vertexShader_glsl_start;
extern char _binary_vertexShader_glsl_end;
extern char _binary_fragmentShader_glsl_start;
extern char _binary_fragmentShader_glsl_end;


std::string_view loadBlobFromBinary(StackArena &arena, const char &start, const char &end)
{
    const char *blob_start = &start;
    const char *blob_end = &end;
    i64 size = std::bit_cast<intptr_t, const char*>(blob_end) -std::bit_cast<intptr_t, const char*>(blob_start);

    char* buffer = arena.arenaPush<char>(size);
    memcpy(buffer, blob_start, size);


    assert(size > 0 );
    return {buffer, static_cast<u64>(size)};
}




i32 initialiseRenderer(Renderer &render_manager)
{
    // RENDERER_ASSERT(glXGetCurrentContext() != nullptr, "Called on thread without a valid context.");

    StackArena shader_data {1024 * 10}; // NOTE: 10 KiB is over allocation
    ModelMetaData metaData;

    glGenVertexArrays(1, &render_manager.dummy_vao);
    

    constexpr i32 point_light_size = MAX_POINT_LIGHTS * sizeof(glmath::Vec4);
    constexpr i32 nDebugEntites = MAX_DEBUG_AABB + MAX_DEBUG_LINES;
    constexpr i32 debug_data_size = sizeof(DebugAABB) * MAX_DEBUG_AABB + sizeof(Line) * MAX_DEBUG_LINES + sizeof(glmath::Vec3) * (nDebugEntites);
    
    LifeTimeArena render_data{point_light_size + debug_data_size};


    // Allocate dynamic data VRAM
    glGenBuffers(1,&render_manager.dynamic_sso);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,render_manager.dynamic_sso);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,render_manager.dynamic_sso);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 32, nullptr,GL_DYNAMIC_DRAW);    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    render_manager.dynamic_sso_capacity = 0;



    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    u32 debug_buffer; // Lifetime, don't store it as it won't be deallocated
    glGenBuffers(1, &debug_buffer);
    glGenVertexArrays(1, &render_manager.debug_vao);
    constexpr i32 debug_vertex_arena_size = sizeof(DebugAABB) * MAX_DEBUG_AABB + sizeof(Line) * MAX_DEBUG_LINES;
    const glmath::Vec3 defaultColour{1.0f, 1.0f, 1.0f};
    render_manager.debug_render_data = SubArena(render_data,debug_vertex_arena_size);
    render_manager.debug_lines = std::span<Line, MAX_DEBUG_LINES>{render_manager.debug_render_data.arenaPushZero<Line>(MAX_DEBUG_LINES), MAX_DEBUG_LINES};
    render_manager.debug_aabb = std::span<DebugAABB, MAX_DEBUG_AABB>{render_manager.debug_render_data.arenaPushZero<DebugAABB>(MAX_DEBUG_AABB), MAX_DEBUG_AABB};
    render_manager.colour = std::span<glmath::Vec3, nDebugEntites>{render_data.arenaPushZero<glmath::Vec3>(nDebugEntites), nDebugEntites};
    for(auto &colour : render_manager.colour) colour = defaultColour;

    glBindVertexArray(render_manager.debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER,debug_buffer);
    glBufferData(GL_ARRAY_BUFFER,render_manager.debug_render_data.offset, render_manager.debug_render_data.start,GL_DYNAMIC_READ);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glmath::Vec3), reinterpret_cast<void*>(0));    
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    auto vert_blob = loadBlobFromBinary(shader_data, _binary_vertexShader_glsl_start, _binary_vertexShader_glsl_end);
    auto frag_blob = loadBlobFromBinary(shader_data, _binary_fragmentShader_glsl_start, _binary_fragmentShader_glsl_end);
    i64 vsObj = compileShader(vert_blob, GL_VERTEX_SHADER);
    i64 fsObj = compileShader(frag_blob, GL_FRAGMENT_SHADER);
    RENDERER_ASSERT(vsObj != -1 && fsObj != -1, "Failed to compile shaders.");

    
    render_manager.shader_program = glCreateProgram();
    glAttachShader(render_manager.shader_program,static_cast<u32>(vsObj));
    glAttachShader(render_manager.shader_program,static_cast<u32>(fsObj));
    glLinkProgram(render_manager.shader_program);
    i32 programCreated;
    glGetProgramiv(render_manager.shader_program,GL_LINK_STATUS,&programCreated);
    if(!programCreated)
    {
        i32 length;
        glGetProgramiv(render_manager.shader_program,GL_INFO_LOG_LENGTH, &length);
        char *log = new char[length];
        glGetProgramInfoLog(render_manager.shader_program,length,NULL,log);
        RENDERER_LOG(log);
        delete[] log;
        return -1;
    }

    render_manager.projection_uniform = glGetUniformLocation(render_manager.shader_program,"projection");
    render_manager.view_uniform = glGetUniformLocation(render_manager.shader_program,"view");
    render_manager.point_light_uniform = glGetUniformLocation(render_manager.shader_program,"point_lights_ws");


    // assert(render_manager.mvp_uniform != -1);
    render_manager.render_mode_uniform = glGetUniformLocation(render_manager.shader_program,"render_mode");
    assert(render_manager.render_mode_uniform != -1);
    render_manager.particle_radius_uniform = glGetUniformLocation(render_manager.shader_program,"radius");
    // assert(render_manager.particle_scale_uniform != -1);
    render_manager.debug_colours_uniform = glGetUniformLocation(render_manager.shader_program,"debugColours");
    // assert(render_manager.debug_colours_uniform != -1);

    // Set shader defaults
    glUseProgram(render_manager.shader_program);
    glUniform1f(render_manager.particle_radius_uniform, 0.005f);



    return 1;
}




void sortParticlesByDepth(Renderer &renderer, const glmath::Vec3 &camera_pos)
{
    
    std::sort(renderer.particle_data.begin(), renderer.particle_data.end(), [camera_pos](const ParticleData &a, const ParticleData &b)
    {
        float distance_a = 0;
        float distance_b = 0;
        for(i32 i = 0 ; i < 3; ++i)
        {
            distance_a += (camera_pos.data[i] - a.position.data[i]) * (camera_pos.data[i] - a.position.data[i]);
            distance_b += (camera_pos.data[i] - b.position.data[i]) * (camera_pos.data[i] - b.position.data[i]);
        }
        return distance_a > distance_b; //distance_a < distance_b will sort in ascending order
    });
}



void renderDebug(const Renderer &renderer)
{
    // Assumes that shader program has been bound

    glBindVertexArray(renderer.debug_vao);
    glBufferSubData(GL_ARRAY_BUFFER, 0, renderer.debug_render_data.offset, renderer.debug_render_data.start); //reupload
    glLineWidth(2);
    constexpr i32 nVerts = MAX_DEBUG_AABB * 24 + MAX_DEBUG_LINES * 2;
    glDrawArrays(GL_LINES, 0 ,nVerts);
    glBindVertexArray(0);
}



void uploadAndRenderParticles(Renderer & renderer)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,renderer.dynamic_sso);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,renderer.dynamic_sso);
    if(renderer.dynamic_sso_capacity < static_cast<i64>(renderer.particle_data.size()))
    {
        i32 new_size_bytes = renderer.particle_data.capacity() * sizeof(ParticleData);
        glBufferData(GL_SHADER_STORAGE_BUFFER, new_size_bytes, nullptr,GL_DYNAMIC_DRAW);
        renderer.dynamic_sso_capacity = static_cast<i64>(renderer.particle_data.capacity());
    }
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, renderer.particle_data.size() * sizeof(ParticleData),renderer.particle_data.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);


    // glDrawElementsInstanced(GL_TRIANGLES, 960 * 3, GL_UNSIGNED_INT, (void*)(0), renderer.particle_data.size());


    glUniform1ui(renderer.render_mode_uniform, 1);
    glDrawArrays(GL_TRIANGLES,0,6 * renderer.particle_data.size());

    
    renderer.particle_data.clear();
}

void renderScene(Renderer & renderer, const glmath::Mat4x4 &view, const glmath::Mat4x4 &projection)
{
    // RENDERER_ASSERT(glXGetCurrentContext() != nullptr, "Called on thread without a valid context.");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderer.shader_program);
    glUniformMatrix4fv(renderer.projection_uniform, 1, false, projection.data[0]);
    glUniformMatrix4fv(renderer.view_uniform, 1, false, view.data[0]);

    renderer.light_pos[0] = glmath::Vec3(3.0, 3.0, 3.0);



    for(i32 i =0; i < 1; ++i)
    {
        renderer.light_pos[0] = glmath::Vec3(view * glmath::Vec4(renderer.light_pos[0], 1.0f)); 
    }

    glUniform3fv(renderer.point_light_uniform, MAX_POINT_LIGHTS, renderer.light_pos[0].data);



    glBindVertexArray(renderer.dummy_vao);
    
    glUniform1ui(renderer.render_mode_uniform, 0);
    glDrawArrays(GL_TRIANGLES,0,6);


    uploadAndRenderParticles(renderer);
    glBindVertexArray(0);

    // glUniform1ui(renderer.render_mode_uniform,0);
    // constexpr i32 nDebugEntites = MAX_DEBUG_AABB + MAX_DEBUG_LINES;
    // glUniform3fv(renderer.debug_colours_uniform, nDebugEntites, renderer.colour[0].data);
    // renderDebug(renderer);
}

void setRadius(const Renderer &renderer, f32 radius)
{
    glUniform1f(renderer.particle_radius_uniform, radius);
}


