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

i64 compileShader(std::string shaderPath, GLenum shaderType)
{
    std::ifstream inf{shaderPath, std::ifstream::binary};
    if(!inf)
        return -1;
    
    inf.seekg(0,std::ios_base::end);
    i32 length = static_cast<i32>(inf.tellg());
    inf.seekg(0,std::ios_base::beg);
    
    char *shaderBlob = new char[length];
    inf.read(shaderBlob,length);
    inf.close();
    
    u32 shaderObj = glCreateShader(shaderType);
    i32 shaderCompiled;
    glShaderSource(shaderObj,1,&shaderBlob,&length);
    glCompileShader(shaderObj);
    glGetShaderiv(shaderObj,GL_COMPILE_STATUS,&shaderCompiled);
    
    delete[] shaderBlob;
    
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

    SubArena dynamic_render_data; 
    std::span<ParticleData> particle_data;
    std::span<i32> particle_colour_index;
    std::span<glmath::Vec4> light_pos;

    u32 dynamic_sso;
    u32 debug_vao;
    u32 sphere_vao;

    i32 debug_colours_uniform;

    i32 shader_program;
    i32 render_mode_uniform;
    i32 mvp_uniform;
    i32 particle_scale_uniform;
};



void uploadParticleData(const Renderer & render_manager)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,render_manager.dynamic_sso);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, render_manager.dynamic_render_data.offset, render_manager.dynamic_render_data.start);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

i32 initialiseRenderer(Renderer &render_manager, i32 n_points)
{
    // load sphere to vram
    char path[] = "../../../data/spherePosNormalTriangulated.ply";

    StackArena sphere_data {1024 * 50}; // NOTE: 50 KiB is over allocation
    ModelMetaData metaData;
    metaData.blob = loadFile(sphere_data, path);
    getPlyMetaData(metaData);
    i64 vertex_data_size = metaData.nVerts * sizeof(VertexPosNormal);
    i64 index_data_size = metaData.nTriangles * 3  * sizeof(i32);
    SubArena vertex_data{sphere_data,vertex_data_size};
    SubArena index_data{sphere_data, index_data_size};

    loadPlyModelPos(vertex_data,index_data,metaData);

    std::string render_msg = std::string("Rendering ") + std::to_string(metaData.nTriangles * n_points) + std::string(" triangles.");
    RENDERER_LOG(render_msg.c_str());

    u32 sphere_vbo; // Lifetime, don't store it as it won't be deallocated
    u32 sphere_ebo; // Lifetime, don't store it as it won't be deallocated
    glGenBuffers(1, &sphere_vbo);
    glGenBuffers(1, &sphere_ebo);
    glGenVertexArrays(1, &render_manager.sphere_vao);
    
    glBindVertexArray(render_manager.sphere_vao);

    glBindBuffer(GL_ARRAY_BUFFER,sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.offset, vertex_data.start,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sphere_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_data.offset,index_data.start,GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3,GL_FLOAT,GL_FALSE,sizeof(VertexPosNormal), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3,GL_FLOAT,GL_FALSE,sizeof(VertexPosNormal), reinterpret_cast<void*>(sizeof(glmath::Vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
  

    // load debug data to vram

    i32 particle_data_size = n_points * sizeof(ParticleData);
    constexpr i32 point_light_size = MAX_POINT_LIGHTS * sizeof(glmath::Vec4);
    constexpr i32 nDebugEntites = MAX_DEBUG_AABB + MAX_DEBUG_LINES;
    constexpr i32 debug_data_size = sizeof(DebugAABB) * MAX_DEBUG_AABB + sizeof(Line) * MAX_DEBUG_LINES + sizeof(glmath::Vec3) * (nDebugEntites);
    
    LifeTimeArena render_data{particle_data_size + point_light_size + debug_data_size};


    // Allocate dynamic data SysRAM
    render_manager.dynamic_render_data = SubArena(render_data, point_light_size + particle_data_size); 
    render_manager.light_pos = std::span<glmath::Vec4>{render_manager.dynamic_render_data.arenaPushZero<glmath::Vec4>(MAX_POINT_LIGHTS), MAX_POINT_LIGHTS};
    render_manager.particle_data = std::span<ParticleData>{render_manager.dynamic_render_data.arenaPushZero<ParticleData>(n_points), static_cast<u64>(n_points)};

    // Allocate dynamic data VRAM
    glGenBuffers(1,&render_manager.dynamic_sso);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,render_manager.dynamic_sso);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,render_manager.dynamic_sso);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_manager.dynamic_render_data.offset,nullptr,GL_DYNAMIC_DRAW);    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


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
    glBufferData(GL_ARRAY_BUFFER,render_manager.debug_render_data.offset, render_manager.debug_render_data.start,GL_STATIC_READ);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glmath::Vec3), reinterpret_cast<void*>(0));    
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    i64 vsObj = compileShader("../../../src/shaders/vertexShader.glsl", GL_VERTEX_SHADER);
    i64 fsObj = compileShader("../../../src/shaders/fragmentShader.glsl", GL_FRAGMENT_SHADER);
    RENDERER_ASSERT(vsObj != -1 && vsObj != -1, "Failed to compile shaders.");

    
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

    render_manager.mvp_uniform = glGetUniformLocation(render_manager.shader_program,"mvp");
    // assert(render_manager.mvp_uniform != -1);
    render_manager.render_mode_uniform = glGetUniformLocation(render_manager.shader_program,"render_mode");
    // assert(render_manager.render_mode_uniform != -1);
    render_manager.particle_scale_uniform = glGetUniformLocation(render_manager.shader_program,"particle_scale");
    // assert(render_manager.particle_scale_uniform != -1);
    render_manager.debug_colours_uniform = glGetUniformLocation(render_manager.shader_program,"debugColours");
    // assert(render_manager.debug_colours_uniform != -1);


    return 1;
}

void sortParticlesByDepth(const Renderer &renderer, glmath::Vec3 camera_pos)
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



void renderParticles(const Renderer & renderer, i32 n_points)
{
    //  glUniform3f(camera_pos_ws_uniform,camera.pos.x,camera.pos.y,camera.pos.z);
    glBindVertexArray(renderer.sphere_vao);
    // GLuint query;
    // glGenQueries(1, &query);
    // glBeginQuery(GL_TIME_ELAPSED, query);
    glDrawElementsInstanced(GL_TRIANGLES, 960 * 3, GL_UNSIGNED_INT, (void*)(0), n_points);
    // glEndQuery(GL_TIME_ELAPSED);
    // GLuint64 elapsed;
    // glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed);
    // printf("Elapsed: %f ms\n", elapsed / 1e6);
    glBindVertexArray(0);

}

void renderScene(const Renderer & renderer, const glmath::Mat4x4 &mvp, i32 n_points)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderer.shader_program);
    glUniformMatrix4fv(renderer.mvp_uniform, 1, false, mvp.data[0]);


    glUniform1ui(renderer.render_mode_uniform,2);
    glUniform1f(renderer.particle_scale_uniform, 0.005f);
    renderParticles(renderer ,n_points);

    // glUniform1ui(renderer.render_mode_uniform,0);
    // constexpr i32 nDebugEntites = MAX_DEBUG_AABB + MAX_DEBUG_LINES;
    // glUniform3fv(renderer.debug_colours_uniform, nDebugEntites, renderer.colour[0].data);
    // renderDebug(renderer);
}


