#include <iostream>
#include <fstream>
#include <string>
#include <Chrono>
#include <span>
#include <cassert>
#include <charconv>
#include <array>

#include "external/glad/glad.h"
#include "external/glfw3.h"

#include "defintions.h"
#include "arena.h"
#include "utility.h"


#define JSMN_PARENT_LINKS



#include "defintions.h"
#include "glmath.h"
#include <algorithm>

constexpr i32 clientWidth = 500;
constexpr i32 clientHeight  = 500;

f64 mouseX = 0.0;
f64 mouseY = 0.0;
f32 deltaMouseX = 0.0;
f32 deltaMouseY = 0.0;

glmath::Vec3 up = {0.0,1.0,0.0};
glmath::Quaternion startDirection = {0.0,0.0,-1.0,0.0};
glmath::Vec3 initalPos = {0.5,0.5,-2.0};

    
void frameBufferCallBack(GLFWwindow * window, i32 width, i32 height)
{
    std::cout<<width<<", "<<height<<"\n";
    glViewport(0,0,width,height);
}

void cursorCallBack(GLFWwindow * window, f64 newX, f64 newY)
{
    deltaMouseX = static_cast<f32>(newX - mouseX);
    deltaMouseY = static_cast<f32>(newY - mouseY);
    mouseX = newX;
    mouseY = newY;
}
    
struct Camera
{
    glmath::Mat4x4 view;
    glmath::Mat4x4 projection; // shouldn't be here cold data
    glmath::Vec3 pos;
    glmath::Vec3 cameraFront;
};



void processUserInput(GLFWwindow * window, Camera &camera)
{
    f32 mouseSensitivity = glmath::PI / 180.0f *0.05f;
    static f32 pitch = 0.0f;
    static f32 yaw = 0.0f;
    static f32 roll = 0.0f;
    constexpr f32 eps = 0.001f;
    constexpr f32 cameraSpeed = 0.001f;

    if(deltaMouseX != 0.0f || deltaMouseY != 0.0f)
    {
        yaw += deltaMouseX*mouseSensitivity;
        pitch +=deltaMouseY*mouseSensitivity;
        pitch = std::clamp(pitch, -glmath::PI/2 + eps, glmath::PI /2 - eps); // lock pitch to avoid co-linear basis with view

        deltaMouseX = 0;
        deltaMouseY = 0;
    }


        // glmath::Quaternion qz = glmath::Quaternion(0.0,0.0,sin(-roll / 2),cos(-roll /2));
        glmath::Quaternion qy = glmath::Quaternion(0.0,sin(-yaw / 2),0.0f,cos(-yaw /2));
        glmath::Quaternion qx = glmath::Quaternion(sin(-pitch / 2),0.0f,0.0f,cos(-pitch /2));


        camera.view = glmath::quaternionToMatrix(qx * qy) * glmath::translate(-camera.pos);

        // Transposed and stored in column-major so its accessed in row-major     
        glmath::Vec3 forwardBasis = {
                                camera.view.data[0][2],
                                camera.view.data[1][2],
                                camera.view.data[2][2]
                                };

        glmath::Vec3 rightBasis  = {
                                camera.view.data[0][0],
                                camera.view.data[1][0],
                                camera.view.data[2][0]
                                };

     

        if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS)
            camera.pos += cameraSpeed * forwardBasis; 
        if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS)
            camera.pos += -cameraSpeed * forwardBasis;
        if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS)
            camera.pos += cameraSpeed * rightBasis;
        if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS)
            camera.pos += -cameraSpeed * rightBasis;
        if(glfwGetKey(window,GLFW_KEY_Q) == GLFW_PRESS)
            roll += mouseSensitivity;
        if(glfwGetKey(window,GLFW_KEY_E) == GLFW_PRESS)
            roll -= mouseSensitivity;
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

        std::cerr<<shaderType<<'\n'<<errorMsg<<std::endl;
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


enum RenderingType: i8
{
    invalidType,
    pos,
    pos_normal,
    pos_uv,
    typeCount
};

struct ModelVertexData
{
    i32 vertex_offset;
    i32 index_offset;
    u32 n_verts;
    u32 n_indices;
    RenderingType rendering_type;
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
        assert(face_number == 3 && "Triangulate mesh.");

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
    VertexPosNormal* vertices_phong = vertexDataArena.arenaPush<VertexPosNormal>(modelInfo.nVerts);
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
        vertices_phong[i] = {
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


struct Renderer
{
    SubArena debug_render_data; 
    std::span<DebugAABB> debug_aabb;
    std::span<Line> debug_lines;
    std::span<glmath::Vec3> colour;

    SubArena dynamic_render_data; 
    std::span<glmath::Vec4> light_pos;
    std::span<glmath::Vec4> particle_pos;

    u32 dynamic_sso;
    u32 debug_vao;
    u32 sphere_vao;
};

void initialiseRenderer(Renderer &render_manager, i32 n_points)
{
    // load sphere to vram
    char path[] = "..\\data\\spherePosNormalTriangulated.ply";

    StackArena sphere_data {1024 * 50}; // NOTE: 50 KiB is over allocation
    ModelMetaData metaData;
    metaData.blob = loadFile(sphere_data, path);
    getPlyMetaData(metaData);
    i64 vertex_data_size = metaData.nVerts * sizeof(VertexPosNormal);
    i64 index_data_size = metaData.nTriangles * 3  * sizeof(i32);
    SubArena vertex_data{sphere_data,vertex_data_size};
    SubArena index_data{sphere_data, index_data_size};

    loadPlyModelPos(vertex_data,index_data,metaData);

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

    i32 particle_data_size = n_points * sizeof(glmath::Vec4);
    constexpr i32 point_light_size = MAX_POINT_LIGHTS * sizeof(glmath::Vec4);
    constexpr i32 nDebugEntites = MAX_DEBUG_AABB + MAX_DEBUG_LINES;
    constexpr i32 debug_data_size = sizeof(DebugAABB) * MAX_DEBUG_AABB + sizeof(Line) * MAX_DEBUG_LINES + sizeof(glmath::Vec3) * (nDebugEntites);
    
    LifeTimeArena render_data{particle_data_size + point_light_size + debug_data_size};


    // use vec4 given that in std340/std130 vec3 has an alignment of 16 bytes

    render_manager.dynamic_render_data = SubArena(render_data, point_light_size + particle_data_size); 

    render_manager.light_pos = std::span<glmath::Vec4>{render_manager.dynamic_render_data.arenaPushZero<glmath::Vec4>(MAX_POINT_LIGHTS), MAX_POINT_LIGHTS};
    render_manager.particle_pos = std::span<glmath::Vec4>{render_manager.dynamic_render_data.arenaPushZero<glmath::Vec4>(n_points), static_cast<u64>(n_points)};

    srand(20);
    i32 dim = 3;
    for(auto &point : render_manager.particle_pos)
        for(i32 i = 0; i < dim; ++i)
            point.data[i] = (static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX));




    render_manager.light_pos[0] = {2.0};

    

    
    
    glGenBuffers(1,&render_manager.dynamic_sso);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,render_manager.dynamic_sso);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_manager.dynamic_render_data.offset,render_manager.dynamic_render_data.start,GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,render_manager.dynamic_sso);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);




    u32 debug_buffer; // Lifetime, don't store it as it won't be deallocated
    glGenBuffers(1, &debug_buffer);
    glGenVertexArrays(1, &render_manager.debug_vao);
    constexpr i32 vertexArenaSize = sizeof(DebugAABB) * MAX_DEBUG_AABB + sizeof(Line) * MAX_DEBUG_LINES;
    const glmath::Vec3 defaultColour{1.0f, 1.0f, 1.0f};
    render_manager.debug_render_data = SubArena(render_data,vertexArenaSize);
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


}
void renderDebug(const Renderer &debugManager, u32 debugColourUniform)
{
    // Assumes that shader program has been bound
    
    constexpr i32 nDebugEntites = MAX_DEBUG_AABB + MAX_DEBUG_LINES;
    glUniform3fv(debugColourUniform, nDebugEntites, debugManager.colour[0].data);
    
    
    glBindVertexArray(debugManager.debug_vao);
    glBufferSubData(GL_ARRAY_BUFFER, 0, debugManager.debug_render_data.offset, debugManager.debug_render_data.start); //reupload



    glLineWidth(1);
    constexpr i32 nVerts = MAX_DEBUG_AABB * 24 + MAX_DEBUG_LINES * 2;
    glDrawArrays(GL_LINES, 0 ,nVerts);

    glBindVertexArray(0);
}








int main()
{   

    const auto programStartTime = std::chrono::steady_clock::now();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(clientWidth,clientHeight,"DoG",NULL,NULL);

    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return 1;

    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window,frameBufferCallBack); //Handle resizing
    glfwSetCursorPosCallback(window,cursorCallBack);
    //Frame Buffer Setup
    glViewport(0, 0, clientWidth, clientHeight);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    
    i64 vsObj = compileShader("..\\src\\Shaders\\vertexShader.glsl", GL_VERTEX_SHADER);
    i64 fsObj = compileShader("..\\src\\Shaders\\fragmentShader.glsl", GL_FRAGMENT_SHADER);
    if(fsObj == -1 || vsObj == -1)
        return 1;
    
    u32 shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram,static_cast<u32>(vsObj));
    glAttachShader(shaderProgram,static_cast<u32>(fsObj));
    glLinkProgram(shaderProgram);
    i32 programCreated;
    glGetProgramiv(shaderProgram,GL_LINK_STATUS,&programCreated);
    if(!programCreated)
    {
        i32 length;
        glGetProgramiv(shaderProgram,GL_INFO_LOG_LENGTH, &length);
        char *log = new char[length];
        glGetProgramInfoLog(shaderProgram,length,NULL,log);
        std::cout<<log;
    }

    i32 mvp_uniform = glGetUniformLocation(shaderProgram,"mvp");
    assert(mvp_uniform != -1);
    i32 debug_colour_uniform = glGetUniformLocation(shaderProgram,"debugColours");
    assert(debug_colour_uniform != -1);
    i32 render_mode_uniform = glGetUniformLocation(shaderProgram,"renderMode");
    assert(render_mode_uniform != -1);
    i32 camera_pos_uniform= glGetUniformLocation(shaderProgram,"camera_pos_ws");
    assert(camera_pos_uniform != -1);
    // i32 light_pos_uniform= glGetUniformLocation(shaderProgram,"light_pos_ws");
    // assert(light_pos_uniform != -1);
    // i32 normal_transform_uniform = glGetUniformLocation(shaderProgram,"normal_transform");
    // assert(normal_transform_uniform != -1);
    i32 particle_scale_uniform = glGetUniformLocation(shaderProgram, "particle_scale");
    assert(particle_scale_uniform != -1);
    


    



    Camera cam = {};
    f32 aspectRatio = clientWidth / clientHeight;
    f32 verticalFOV  = 45 * glmath::PI / 180;
    f32 near_plane = 0.1f;
    f32 far_plane = 1000.0f;
    glmath::Mat4x4 projection = glmath::perspectiveProjection(verticalFOV,aspectRatio,near_plane,far_plane);
    cam.projection = projection;
    cam.pos = initalPos;
    cam.cameraFront = startDirection.getVectorComponent();


    i32 n_points = 1000;
    Renderer renderer;
    initialiseRenderer(renderer, n_points);
    const auto reachRenderLoopTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> launchTime = reachRenderLoopTime - programStartTime;
    constexpr i32 titleBarMaxLength = 50;
    char titleBarString[titleBarMaxLength];
    i32 launchTimeLength = snprintf(titleBarString,titleBarMaxLength,"Launch: %3fms",launchTime.count() * 1000.0);
    
    glmath::Mat4x4 mvp;
    glmath::Vec3 light_pos{6.0f,6.0f,6.0f};

    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);


    f32 elapsed_time = 0.0;
    constexpr f32 dt = 0.001;
    while(!glfwWindowShouldClose(window))
    {
        
        const auto frameStart =  std::chrono::steady_clock::now();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glfwPollEvents();
        
        processUserInput(window,cam);
        
        
        
        glUseProgram(shaderProgram);

        mvp = cam.projection * cam.view;
        glUniformMatrix4fv(mvp_uniform, 1, false, mvp.data[0]);


        // f32 size = 3.0f;
        // renderer.debug_lines[0] = {{-size, 0.0f, -size}, {size, 0.0f, -size}};
        // renderer.debug_lines[1] = {{size, 0.0f, -size}, {size, 0.0f, size}};
        // renderer.debug_lines[2] = {{size, 0.0f, size}, {-size, 0.0f, size}};
        // renderer.debug_lines[3] = {{-size, 0.0f, size}, {-size, 0.0f, -size}};

        // renderer.colour[0] = {1.0,0.0,0.0};
        // renderer.colour[1] = {1.0,0.0,0.0};
        // renderer.colour[2] = {1.0,0.0,0.0};
        // renderer.colour[3] = {1.0,0.0,0.0};

        renderer.debug_aabb[0] = {{0.0},{1.0}};
        renderer.colour[MAX_DEBUG_LINES] = {1.0,0.0,0.0};


        
        mvp = cam.projection * cam.view;
        
        glUniform1ui(render_mode_uniform,0);        
        glUniformMatrix4fv(mvp_uniform, 1, false, mvp.data[0]);
        renderDebug(renderer, debug_colour_uniform);

        glUniform3f(camera_pos_uniform, cam.pos.x,cam.pos.y,cam.pos.z);
        // glUniform3f(light_pos_uniform, light_pos.x,light_pos.y,light_pos.z);
        // glUniformMatrix4fv(normal_transform_uniform,1,false,model.data[0]);

        // glUniform1f(particle_scale_uniform, 0.06f);
        
        
        glBindVertexArray(renderer.sphere_vao);
        
        renderer.light_pos[0] = {cos(elapsed_time), 1.5, sin(elapsed_time),0.0};

        glBindBuffer(GL_SHADER_STORAGE_BUFFER,renderer.dynamic_sso);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,renderer.dynamic_render_data.offset,renderer.dynamic_render_data.start);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);

        
        glUniform1ui(render_mode_uniform,1);
        glUniform1f(particle_scale_uniform, 0.1f);
        glDrawElementsInstanced(GL_TRIANGLES,960 * 3,GL_UNSIGNED_INT, static_cast<void*>(0),MAX_POINT_LIGHTS);

        glUniform1ui(render_mode_uniform,2);
        glUniform1f(particle_scale_uniform, 0.02f);
        glDrawElementsInstanced(GL_TRIANGLES,960 * 3,GL_UNSIGNED_INT, static_cast<void*>(0),n_points);


        glBindVertexArray(0);


        glfwSwapBuffers(window);
        const auto frameEnd =  std::chrono::steady_clock::now();
        const std::chrono::duration<double> frameDuration = frameEnd - frameStart;

        snprintf(&titleBarString[launchTimeLength],titleBarMaxLength - launchTimeLength - 1," Frame: %f3ms", frameDuration.count() * 1000.0);
        glfwSetWindowTitle(window,titleBarString);

        elapsed_time += dt;
    }
     
    glfwTerminate();
    
    return 0;
} 