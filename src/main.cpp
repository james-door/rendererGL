#include <iostream>
#include <cassert>
#include <fstream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


#include "external/glad/glad.h"
#include <GL/glx.h>


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
    Display* connection = XOpenDisplay(nullptr);
    i32 default_screen = DefaultScreen(connection);
    Window root_window = RootWindow(connection,default_screen);

    constexpr i32 tl_window_x = 200;
    constexpr i32 tl_window_y = 200;
    constexpr i32 client_width = 1024;
    constexpr i32 client_height = 1024;
    constexpr i32 border_width = 4;


    
    
    i32 attribList[] = {GLX_RGBA,GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo* visual_info = glXChooseVisual(connection, default_screen, attribList);
    assert(visual_info != nullptr);
    
    
    // int n_visuals;
    // XVisualInfo* visual_info = XGetVisualInfo(connection, VisualNoMask, nullptr, &n_visuals);
    // Visual* visual = visual_info[0].visual;
    // i32 bit_depth = visual_info[0].depth; // True colour

    u64 attirubte_mask = CWEventMask | CWColormap;
    XSetWindowAttributes window_attributes;
    window_attributes.colormap = XCreateColormap(connection,root_window,visual_info->visual,AllocNone);
    window_attributes.event_mask = ExposureMask | KeyPressMask;


    Window window = XCreateWindow(connection,root_window,tl_window_x, tl_window_y, client_width, client_height, border_width, visual_info->depth,InputOutput,visual_info->visual,attirubte_mask,&window_attributes);

    XSelectInput(connection,window,ExposureMask | KeyPressMask);

    XMapWindow(connection,window);
    
    // XFlush(connection);
    // XSetWindowBackground(connection, window, 0x00FF00);

    GLXContext context = glXCreateContext(connection, visual_info, NULL, true);
    assert(context != nullptr);
    bool context_created = glXMakeCurrent(connection, window, context);
    assert(context_created == true);


    gladLoadGLLoader((GLADloadproc)glXGetProcAddress);




    
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
    





    
    glClearColor(1.0,0.0,0.0,1.0);
    glViewport(0,0,client_width,client_height);

    
    
    
    XFree(visual_info);
    
    XEvent event;
    while(1)
    {
        if(XPending(connection) > 0)
        XNextEvent(connection,&event);
        
        if(event.type == Expose)
        {
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shader_program);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES,0,3);

            glBindVertexArray(0);


            glXSwapBuffers(connection,window);

        }
        

    }


}
