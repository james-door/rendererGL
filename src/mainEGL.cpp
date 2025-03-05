#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>


// egl
#include <EGL/egl.h>

// external
#include "external/glad/glad.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "defintions.h"
#include "glmath.h"

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






int main() {
    constexpr i32 client_width = 1024;
    constexpr i32 client_height = 1024;

    const EGLint attribute_list[] = {
            EGL_RED_SIZE, 1,
            EGL_GREEN_SIZE, 1,
            EGL_BLUE_SIZE, 1,
            EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_API);

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(display != EGL_NO_DISPLAY);
    EGLBoolean initalisationSuccess = eglInitialize(display, NULL, NULL);
    assert(initalisationSuccess == EGL_TRUE);

    EGLConfig config;
    i32 num_config;

    eglChooseConfig(display, attribute_list, &config, 1, &num_config);
    assert(num_config > 0);

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);

    assert(context != EGL_NO_CONTEXT);

    
    EGLint offscreen_buffer_attributes[] = {EGL_HEIGHT, client_height, EGL_WIDTH, client_width,EGL_NONE};
    EGLSurface offscreen_surface = eglCreatePbufferSurface(display,config,offscreen_buffer_attributes);
    assert(offscreen_surface != EGL_NO_SURFACE);

    EGLBoolean context_creation_success = eglMakeCurrent(display, offscreen_surface, offscreen_surface, context);
    assert(context_creation_success);
    

    i32 glad_load_success = gladLoadGLLoader((GLADloadproc)eglGetProcAddress);
    assert(glad_load_success);
    

    
    glmath::Vec2 triangle_ndc[3] = {
                                     {0.5, -0.5}, // BR
                                     {0.0, 0.5}, // T
                                     {-0.5, -0.5} // BL
                                   };




    u32 vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_ndc),triangle_ndc,GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2,GL_FLOAT,GL_FALSE,0,(void*)(0));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    
    // load shaders 
    i64 vsObj = compileShader("../src/shaders/vertexShader.glsl", GL_VERTEX_SHADER);
    i64 fsObj = compileShader("../src/shaders/fragmentShader.glsl", GL_FRAGMENT_SHADER);
    if(fsObj == -1 || vsObj == -1)
        return 1;
    
    u32 shader_program = glCreateProgram();

    glAttachShader(shader_program,static_cast<u32>(vsObj));
    glAttachShader(shader_program,static_cast<u32>(fsObj));
    glLinkProgram(shader_program);
    i32 programCreated;
    glGetProgramiv(shader_program,GL_LINK_STATUS,&programCreated);
    if(!programCreated)
    {
        i32 length;
        glGetProgramiv(shader_program,GL_INFO_LOG_LENGTH, &length);
        char *log = new char[length];
        glGetProgramInfoLog(shader_program,length,NULL,log);
        std::cout<<log;
    }
    



    stbi_flip_vertically_on_write(true);
    glClearColor(1.0,0.0,0.0,1.0);
    // glViewport(0,0,client_width,client_height);

    std::vector<u8> colour_buffer(client_width * client_height * 3);
    while(1)
    {
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shader_program);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES,0,3);

            glBindVertexArray(0);

            glReadPixels(0,0,client_width, client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
            stbi_write_png("test.png",client_width,client_height,3,colour_buffer.data(), client_width * 3);
            

            eglSwapBuffers(display,offscreen_surface);
    }


}
