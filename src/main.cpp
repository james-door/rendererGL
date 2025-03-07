#include <iostream>
#include <filesystem>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


#include "external/glad/glad.h"
#include "external/glad/glad_glx.h"



#include "renderer.cpp"

#include "defintions.h"
#include "glmath.h"



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
    f32 pitch;
    f32 yaw;
};

struct WindowState
{
    Display *connection;
    Window window;
    i32 client_width;
    i32 client_height;
    i32 mouse_x;
    i32 mouse_y;
};


void handleUserInput(XEvent &event, Camera& camera, WindowState &window_state, glmath::Mat4x4 &view)
{

    constexpr f32 mouseSensitivity = glmath::PI / 180.0f *0.05f;
    constexpr f32 camera_speed = 0.1f;
    constexpr f32 eps = 0.001f;
    static bool first_input = true;

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
        // std::cout<<XKeysymToString(pressed_key)<<"\n";
    }

    i32 delta_x = 0;
    i32 delta_y = 0;
    i32 bound = 50;
    if(event.type == MotionNotify)
    {   
        i32 current_mouse_x = event.xmotion.x;
        i32 current_mouse_y = event.xmotion.y;

        if(current_mouse_x> window_state.client_width - bound || current_mouse_x < bound)
        {
            i32 temp_delta_x = current_mouse_x - window_state.mouse_x;
            XWarpPointer(window_state.connection,window_state.window, window_state.window,0,0,0,0, window_state.client_width/2,current_mouse_y);
            current_mouse_x = window_state.client_width/2 + temp_delta_x;
            window_state.mouse_x = window_state.client_width/2;
        }
        if(current_mouse_y > window_state.client_height - bound || current_mouse_y < bound)
        {
            i32 temp_delta_y = current_mouse_y - window_state.mouse_y;
            XWarpPointer(window_state.connection,window_state.window, window_state.window,0,0,0,0,current_mouse_x, window_state.client_height / 2);
            current_mouse_y = window_state.client_height/2 + temp_delta_y;
            window_state.mouse_y = window_state.client_height/2;
        }



        if(first_input)
        {
            window_state.mouse_x = current_mouse_x;
            window_state.mouse_y = current_mouse_y;
            first_input = false;            
        }
        if(!(current_mouse_x == window_state.mouse_x && event.xmotion.y - window_state.mouse_y))
        {
            delta_x = current_mouse_x - window_state.mouse_x;
            delta_y = event.xmotion.y - window_state.mouse_y;
            window_state.mouse_x = current_mouse_x;
            window_state.mouse_y = current_mouse_y;
        }

    }

    camera.yaw += delta_x * mouseSensitivity;
    camera.pitch +=delta_y * mouseSensitivity;
    camera.pitch = std::clamp(camera.pitch, -glmath::PI/2 + eps, glmath::PI /2 - eps); // lock pitch to avoid co-linear basis with view

    glmath::Quaternion qy = glmath::Quaternion(0.0,sin(-camera.yaw / 2),0.0f,cos(-camera.yaw /2));
    glmath::Quaternion qx = glmath::Quaternion(sin(-camera.pitch / 2),0.0f,0.0f,cos(-camera.pitch /2));
    view = glmath::quaternionToMatrix(qx * qy) *  glmath::translate(-camera.pos);;
    

}


WindowState createX11WindowAndContext()
{
    constexpr i32 tl_window_x = 200;
    constexpr i32 tl_window_y = 200;
    constexpr i32 client_width = 1024;
    constexpr i32 client_height = 1024;
    constexpr i32 border_width = 4;
    constexpr i32 fb_attributes[] = {GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_RED_SIZE, 8,GLX_GREEN_SIZE, 8,GLX_BLUE_SIZE, 8,GLX_ALPHA_SIZE, 8,GLX_DEPTH_SIZE, 24,GLX_DOUBLEBUFFER, True, None};
    constexpr int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 4,GLX_CONTEXT_MINOR_VERSION_ARB, 3, GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None};
    
    constexpr i32 log_buffer_max_len = 120;
    char log_buffer[log_buffer_max_len];


    XSetErrorHandler(d11_error_handler);
    Display* connection = XOpenDisplay(nullptr); //X11 allocates


    i32 default_screen = DefaultScreen(connection);
    Window root_window = RootWindow(connection,default_screen);


    gladLoadGLX(connection,default_screen);
    

    i32 n_fb_valid_configs;
    GLXFBConfig *fb_configs = glXChooseFBConfig(connection,default_screen, fb_attributes, &n_fb_valid_configs); 
    RENDERER_ASSERT(fb_configs != nullptr, "Failed to find valid framebuffer config.");


    snprintf(log_buffer,log_buffer_max_len,"%d possible frame buffer configurations", n_fb_valid_configs);
    RENDERER_LOG(log_buffer);


    // create a visual from the FBC using glXGetVisualFromFBConfig
    GLXFBConfig chosen_fb_config = fb_configs[0]; // just choose first one
    XVisualInfo *visual_info = glXGetVisualFromFBConfig(connection, chosen_fb_config);

    u64 attirubte_mask = CWEventMask | CWColormap;
    long input_mask = ExposureMask | KeyPressMask | PointerMotionMask;
    XSetWindowAttributes window_attributes;
    window_attributes.colormap = XCreateColormap(connection,root_window,visual_info->visual,AllocNone);
    window_attributes.event_mask = input_mask;
    Window window = XCreateWindow(connection,root_window,tl_window_x, tl_window_y, client_width, client_height, border_width, visual_info->depth,InputOutput,visual_info->visual,attirubte_mask,&window_attributes);


    XSelectInput(connection,window, input_mask);
    XMapWindow(connection,window);
        
    GLXContext context = glXCreateContextAttribsARB(connection, chosen_fb_config,NULL,true,context_attribs);
    RENDERER_ASSERT(context != nullptr, "Could not create OpenGL context.");
    bool current_context_success = glXMakeCurrent(connection, window, context);
    RENDERER_ASSERT(current_context_success == true, "Failed to make the context active.");
    

    XFree(fb_configs);
    XFree(visual_info);
    return {connection, window,client_width,client_height,0,0};
}


// TOOD: dummy python program interfacing with c++
// TOOD: dummy taichi  program interfacing with C++
// TOOD: renderer with dummy taichi
// TODO: renderer with mpm solver

// Optimisations: 
// Fixed Camera 100,000 particles
// With culling: 157524
// Without culling: 247351 

// Depth test enabled: 157524 (same as with culling) [ascending]
// Depth test enabled: 991801 (same as with culling) [descending]
// Depth test disabled: 997043 [ascending]






int main()
{
    std::string cwd = std::string("Current Working Directory: ") + std::filesystem::current_path().string();
    RENDERER_LOG(cwd.c_str());

    const auto programStartTime = std::chrono::steady_clock::now();

    WindowState window_state = createX11WindowAndContext();

    gladLoadGLLoader((GLADloadproc)glXGetProcAddress);

    const u8 *version = glGetString(GL_VERSION);
    const char* version_cstr = reinterpret_cast<const char*>(version);
    RENDERER_LOG(version_cstr);


    bool fixed_camera = True;

    f32 vertical_fov = 45.0 * glmath::PI / 180.0;
    f32 near_plane = 0.1f;
    f32 far_plane  = 1000.f;
    f32 aspect_ratio = static_cast<f32>(window_state.client_width) / static_cast<f32>(window_state.client_height);
    glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);


    Camera camera {};
    glmath::Mat4x4 mvp; 
    glmath::Mat4x4 view = glmath::identity();
    if(fixed_camera)
    {
        camera.pos = {0.5,0.5,-3.0};
        glmath::Vec3 look_at = {0.5,0.5,0.0};
        constexpr glmath::Vec3 up = {0.0,1.0,0.0};
        view = glmath::lookAt(camera.pos,look_at,up);
        mvp = projection * view;
    }
        


    const auto reachRenderLoopTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> launchTime = reachRenderLoopTime - programStartTime;
    constexpr i32 titleBarMaxLength = 100;
    char titleBarString[titleBarMaxLength];
    i32 launchTimeLength = snprintf(titleBarString,titleBarMaxLength,"Launch: %3fms",launchTime.count() * 1000.0);

    XEvent event;
    Window focused_window;
    i32 revert;

    // Hide cursor while window is active
    // static char invisible_data[] = { 0};
    // Pixmap blank = XCreateBitmapFromData(connection, window, invisible_data, 1, 1);
    // XColor dummy = {};
    // Cursor cur = XCreatePixmapCursor(connection, blank, blank, &dummy, &dummy, 0, 0);
    // XDefineCursor(connection, window, cur);
    // XFreePixmap(connection, blank);

    //inital opengl state
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  



    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS); 

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);


    glClearColor(1.0,1.0,1.0,1.0);
    glViewport(0, 0, window_state.client_width, window_state.client_height);
    
    
    i32 n_points = 1000000;
    Renderer renderer = {};
    initialiseRenderer(renderer, n_points);
    srand(20);
    i32 dim = 3;
    for(auto &point : renderer.particle_data)
    {
        for(i32 i = 0; i < dim; ++i)
            point.position.data[i] = (static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX));
        
        // float transparent = static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX);
        // if(transparent < 0.5)
        //     point.colour = {1.0, 0.0, 0.0, 1.0};
        // else
        //     point.colour = {0.0, 0.0, 1.0, 0.3};
        point.colour = {1.0, 0.0, 0.0, 1.0};

    }   
    renderer.light_pos[0] = {2.0};
    


    
    sortParticlesByDepth(renderer, camera.pos);
    uploadParticleData(renderer);

    
    GLuint queryBefore;
    glGenQueries(1, &queryBefore);
    GLuint samplesBefore;
    glGetQueryObjectuiv(queryBefore, GL_QUERY_RESULT, &samplesBefore);
    while(1)
    {

        const auto frameStart =  std::chrono::steady_clock::now();

        // XGetInputFocus(window_state.connection,&focused_window,&revert);
        // if(focused_window == window_state.window)
        // {
        //     XGrabPointer(window_state.connection, window_state.window, False, PointerMotionMask, GrabModeAsync, GrabModeAsync, window_state.window, None, CurrentTime);
        // }
        // else
        // {
        //     XUngrabPointer(window_state.connection,CurrentTime);
        // }

        while (XPending(window_state.connection)) 
        {   
            XNextEvent(window_state.connection, &event);
            if(!fixed_camera) handleUserInput(event, camera, window_state, view);
        }


        mvp = projection * view;
        renderer.debug_aabb[0] = {{0.0,0.0,0.0}, {1.0,1.0,1.0}};
        renderer.colour[MAX_DEBUG_LINES] = {0.0,1.0,0.0};


        glBeginQuery(GL_SAMPLES_PASSED, queryBefore);

        renderScene(renderer,mvp,n_points);
        glXSwapBuffers(window_state.connection,window_state.window);
        
        glEndQuery(GL_SAMPLES_PASSED);
        glGetQueryObjectuiv(queryBefore, GL_QUERY_RESULT, &samplesBefore);





         const auto frameEnd =  std::chrono::steady_clock::now();
         const std::chrono::duration<double> frameDuration = frameEnd - frameStart;
         snprintf(&titleBarString[launchTimeLength],titleBarMaxLength - launchTimeLength - 1," Frame: %f3ms Samples: %d", frameDuration.count() * 1000.0, samplesBefore);
         XStoreName(window_state.connection, window_state.window, titleBarString);
    }

}

