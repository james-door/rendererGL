#include <iostream>
#include <cassert>
#include <fstream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


#include "external/glad/glad.h"
#include <GL/glx.h>


#include "renderer.cpp"

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


int d11_error_handler(Display *connection, XErrorEvent *event)
{
    constexpr i32 max_error_length = 256;
    char error[max_error_length];
    XGetErrorText(connection,event->error_code,error,max_error_length);
    std::cerr<<error<<std::endl;
    return 0;
}



struct Camera
{
    glmath::Vec3 pos;
    i32 mouse_x;
    i32 mouse_y;
    f32 pitch;
    f32 yaw;
};

void handle_input(XEvent &event, Camera& camera, glmath::Mat4x4 &view)
{

    constexpr f32 mouseSensitivity = glmath::PI / 180.0f *0.05f;
    constexpr f32 camera_speed = 0.1f;
    constexpr f32 eps = 0.001f;
    static bool first_input = true;



    if(event.type == MotionNotify)
    {
        if(first_input)
        {
            camera.mouse_x = event.xmotion.x;
            camera.mouse_y = event.xmotion.y;
            first_input = false;            
        }
        if(!(event.xmotion.x == camera.mouse_x && event.xmotion.y - camera.mouse_y))
        {
            int delta_x = event.xmotion.x - camera.mouse_x;
            int delta_y = event.xmotion.y - camera.mouse_y;
            camera.yaw += delta_x * mouseSensitivity;
            camera.pitch +=delta_y * mouseSensitivity;
            camera.pitch = std::clamp(camera.pitch, -glmath::PI/2 + eps, glmath::PI /2 - eps); // lock pitch to avoid co-linear basis with view

            camera.mouse_x = event.xmotion.x;
            camera.mouse_y = event.xmotion.y;
        }
        glmath::Quaternion qy = glmath::Quaternion(0.0,sin(-camera.yaw / 2),0.0f,cos(-camera.yaw /2));
        glmath::Quaternion qx = glmath::Quaternion(sin(-camera.pitch / 2),0.0f,0.0f,cos(-camera.pitch /2));
        view = glmath::quaternionToMatrix(qx * qy) *  glmath::translate(-camera.pos);;
        
    }
    if(event.type == KeyPress)
    {
        KeySym pressed_key = XLookupKeysym(&event.xkey,0);
        glmath::Vec3 forwardBasis = {
                    view.data[0][2],
                    view.data[1][2],
                    view.data[2][2]
                    };
        glmath::Vec3 rightBasis  = {
                    view.data[0][0],
                    view.data[1][0],
                    view.data[2][0]
                    };

        if(pressed_key == XK_w)
        {
            camera.pos += camera_speed * forwardBasis; 
        }
        if(pressed_key == XK_s)
        {
            camera.pos += -camera_speed * forwardBasis; 
        }
        if(pressed_key == XK_a)
        {
            camera.pos += -camera_speed * rightBasis; 
        }
        if(pressed_key == XK_d)
        {
            camera.pos += camera_speed * rightBasis; 
        }
        std::cout<<XKeysymToString(pressed_key)<<"\n";
    }

}




int main() {

    const auto programStartTime = std::chrono::steady_clock::now();

    XSetErrorHandler(d11_error_handler);
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
    

    u64 attirubte_mask = CWEventMask | CWColormap;
    long input_mask = ExposureMask | KeyPressMask | PointerMotionMask;
    XSetWindowAttributes window_attributes;
    window_attributes.colormap = XCreateColormap(connection,root_window,visual_info->visual,AllocNone);
    window_attributes.event_mask = input_mask;


    Window window = XCreateWindow(connection,root_window,tl_window_x, tl_window_y, client_width, client_height, border_width, visual_info->depth,InputOutput,visual_info->visual,attirubte_mask,&window_attributes);


    XSelectInput(connection,window, input_mask);
 


    XMapWindow(connection,window);

    
    GLXContext context = glXCreateContext(connection, visual_info, NULL, true);
    assert(context != nullptr);
    bool context_created = glXMakeCurrent(connection, window, context);
    assert(context_created == true);


    gladLoadGLLoader((GLADloadproc)glXGetProcAddress);

    
    // glmath::Vec2 triangle_ndc[3] = {
    //                                  {0.5, -0.5}, // BR
    //                                  {0.0, 0.5}, // T
    //                                  {-0.5, -0.5} // BL
    //                                };




    // u32 vao, vbo;
    // glGenVertexArrays(1, &vao);
    // glGenBuffers(1, &vbo);

    // glBindVertexArray(vao);
    // glBindBuffer(GL_ARRAY_BUFFER,vbo);
    // glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_ndc),triangle_ndc,GL_STATIC_DRAW);

    // glVertexAttribPointer(0, 2,GL_FLOAT,GL_FALSE,0,(void*)(0));
    // glEnableVertexAttribArray(0);

    // glBindVertexArray(0);

    
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
    



    Renderer renderer = {};
    initialiseRenderer(renderer, 100);

    f32 vertical_fov = 45.0 * glmath::PI / 180.0;
    f32 near_plane = 0.1f;
    f32 far_plane  = 1000.f;
    f32 aspect_ratio = static_cast<f32>(client_width) / static_cast<f32>(client_height);
    glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);

    i32 mvp_uniform = glGetUniformLocation(shader_program,"mvp");
    assert(mvp_uniform != -1);


    glClearColor(1.0,0.0,0.0,1.0);
    glViewport(0,0,client_width,client_height);
    
    XFree(visual_info);
    
    
    glmath::Mat4x4 model = glmath::translate({0.0, 0.0, 10.0});
    
    
    glmath::Mat4x4 mvp; 
    
    XEvent event;

    Camera camera {};
    glmath::Mat4x4 view = glmath::identity();

    // XGrabPointer(connection, window, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
    //     GrabModeAsync, GrabModeAsync, window, None, CurrentTime);


    const auto reachRenderLoopTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> launchTime = reachRenderLoopTime - programStartTime;
    constexpr i32 titleBarMaxLength = 50;
    char titleBarString[titleBarMaxLength];
    i32 launchTimeLength = snprintf(titleBarString,titleBarMaxLength,"Launch: %3fms",launchTime.count() * 1000.0);
    
    while(1)
    {
        const auto frameStart =  std::chrono::steady_clock::now();

        if(XPending(connection) > 0)
        {
            XNextEvent(connection,&event);

            handle_input(event, camera, view);
            // if(event.type == Expose)
            // {


            // }

         }

         glClear(GL_COLOR_BUFFER_BIT);

         glUseProgram(shader_program);

         mvp = projection * view * model;
         glUniformMatrix4fv(mvp_uniform, 1, false, mvp.data[0]);
         glBindVertexArray(renderer.sphere_vao);

         glDrawElements(GL_TRIANGLES, 960 * 3, GL_UNSIGNED_INT, (void*)(0));


         glBindVertexArray(0);

         glXSwapBuffers(connection,window);
         const auto frameEnd =  std::chrono::steady_clock::now();
         const std::chrono::duration<double> frameDuration = frameEnd - frameStart;
         snprintf(&titleBarString[launchTimeLength],titleBarMaxLength - launchTimeLength - 1," Frame: %f3ms", frameDuration.count() * 1000.0);
         XStoreName(connection, window, titleBarString);
    }


    



}
