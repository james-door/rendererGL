// BUGS:
// 1. Breaks most of the time when using debugpy


#if PYTHON_BINDING
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#endif


#include <string>
#include <filesystem>

#include <X11/Xlib.h>
#include "external/glad/glad_glx.h"


#include "defintions.h" 
#include "glmath.h"
#include "renderer.cpp"



struct SurfaceState
{
    Display *connection;
    Window window;
    GLXFBConfig fb_config; //for debugging
    bool open;
    i32 client_width;
    i32 client_height;
    i32 mouse_x;
    i32 mouse_y;
};


int d11_error_handler(Display *connection, XErrorEvent *event)
{
    constexpr i32 max_error_length = 256;
    char error[max_error_length];
    XGetErrorText(connection,event->error_code,error,max_error_length);
    std::cerr<<error<<std::endl;
    return 0;
}

SurfaceState createSurfaceAndContext()
{
    constexpr i32 tl_window_x = 200;
    constexpr i32 tl_window_y = 200;
    constexpr i32 client_width = 1024;
    constexpr i32 client_height = 1024;
    constexpr i32 border_width = 4;
    constexpr i32 fb_attributes[] = {GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_RED_SIZE, 8,GLX_GREEN_SIZE, 8,GLX_BLUE_SIZE, 8,GLX_ALPHA_SIZE, 8,GLX_DEPTH_SIZE, 24,GLX_DOUBLEBUFFER, True, None};
    constexpr int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 4,GLX_CONTEXT_MINOR_VERSION_ARB, 3, GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None};
    


    XSetErrorHandler(d11_error_handler);
    Display* connection = XOpenDisplay(nullptr); //X11 allocates


    i32 default_screen = DefaultScreen(connection);

    Window root_window = RootWindow(connection,default_screen);


    gladLoadGLX(connection,default_screen);
    

    i32 n_fb_valid_configs;
    GLXFBConfig *fb_configs = glXChooseFBConfig(connection,default_screen, fb_attributes, &n_fb_valid_configs); 
    RENDERER_ASSERT(fb_configs != nullptr, "Failed to find valid framebuffer config.");




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
    XMapWindow(connection, window);
        
    GLXContext context = glXCreateContextAttribsARB(connection, chosen_fb_config,NULL,true,context_attribs);
    RENDERER_ASSERT(context != nullptr, "Could not create OpenGL context.");
    bool current_context_success = glXMakeCurrent(connection, window, context);
    RENDERER_ASSERT(current_context_success == true, "Failed to make the context active.");
    

    XFree(fb_configs);
    XFree(visual_info);
    return {connection, window, chosen_fb_config, true, client_width, client_height, 0, 0};
}






struct Camera
{
    glmath::Mat4x4 view;
    glmath::Vec3 pos;
    f32 pitch;
    f32 yaw;
};


void handleUserInput(XEvent &event, Camera& camera, SurfaceState &window_state)
{
    constexpr f32 mouseSensitivity = glmath::PI / 180.0f *0.05f;
    constexpr f32 camera_speed = 0.1f;
    constexpr f32 eps = 0.001f;
    static bool first_input = true;

    if(static_cast<unsigned long>(event.xclient.data.l[0]) == XInternAtom(window_state.connection, "WM_DELETE_WINDOW", False))
            window_state.open = false; 
    if(event.type == KeyPress)
    {
        KeySym pressed_key = XLookupKeysym(&event.xkey,0);
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

    camera.yaw +=   delta_x * mouseSensitivity;
    camera.pitch += delta_y * mouseSensitivity;
    camera.pitch = std::clamp(camera.pitch, -glmath::PI/2 + eps, glmath::PI /2 - eps); // lock pitch to avoid co-linear basis with view

    glmath::Quaternion qy = glmath::Quaternion(0.0,sin(-camera.yaw / 2),0.0f,cos(-camera.yaw /2));
    glmath::Quaternion qx = glmath::Quaternion(sin(-camera.pitch / 2),0.0f,0.0f,cos(-camera.pitch /2));
    camera.view = glmath::quaternionToMatrix(qx * qy) *  glmath::translate(-camera.pos);
    
}






struct GlRenderer
{
    SurfaceState surface_state;
    Camera camera;
    Renderer renderer;
    GlRenderer()
    {
        RENDERER_LOG("Current Working Directory: %s", std::filesystem::current_path().c_str());
        const auto programStartTime = std::chrono::steady_clock::now();
        
        surface_state = createSurfaceAndContext();

        i32 gl_load_status = gladLoadGLLoader((GLADloadproc)glXGetProcAddress);
        RENDERER_ASSERT(gl_load_status == 1,"Failed to load GL with GLAD.");
        const u8 *version = glGetString(GL_VERSION);
        const char* version_cstr = reinterpret_cast<const char*>(version);
        RENDERER_LOG(version_cstr);



        
        renderer = {};
        initialiseRenderer(renderer);


        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);
        glClearColor(1.0,1.0,1.0,1.0);
        glViewport(0, 0, surface_state.client_width, surface_state.client_height);
        
        
        const auto reachRenderLoopTime = std::chrono::steady_clock::now();
        const std::chrono::duration<double> launchTime = reachRenderLoopTime - programStartTime;
        constexpr i32 titleBarMaxLength = 100;
        char titleBarString[titleBarMaxLength];
        snprintf(titleBarString,titleBarMaxLength,"Launch: %3fms",launchTime.count() * 1000.0);
        RENDERER_LOG(titleBarString);
    }


    void processWindowInput()
    {
        XEvent event;
        while (XPending(surface_state.connection)) 
        {   
            XNextEvent(surface_state.connection, &event);
            handleUserInput(event, camera, surface_state);
        }

    }

    bool windowIsClosed()
    {
        return !surface_state.open;
    }



#if PYTHON_BINDING
    void particles(nanobind::ndarray<f32, nanobind::shape<-1, 3>>& centres, nanobind::ndarray<f32, nanobind::shape<-1, 4>>& colours)
    {
        RENDERER_ASSERT((centres.ndim() == 2), "Expected array to be dimension %d",centres.ndim());
        // RENDERER_LOG("Device ID = %u (cpu=%i, cuda=%i)\n", a.device_id(),int(a.device_type() == nanobind::device::cpu::value),int(a.device_type() == nanobind::device::cuda::value));
        
        i64 points_to_allocate = centres.shape(0);
        for (i64 i = 0; i < points_to_allocate; ++i)
        {
            glmath::Vec4 pos = {centres(i, 0), centres(i, 1), centres(i,2), 0.0};
            glmath::Vec4 colour = {colours(i, 0), colours(i, 1), colours(i, 2), colours(i, 3)};
            renderer.particle_data.push_back({pos, colour});
        }
    }

    void setCamera(nanobind::ndarray<f32, nanobind::shape<3>> np_pos, nanobind::ndarray<f32, nanobind::shape<3>> np_lookat)
    {
        glmath::Vec3 pos = {np_pos(0), np_pos(1), np_pos(2)};
        glmath::Vec3 lookat= {np_lookat(0), np_lookat(1), np_lookat(2)};
        camera.pos = pos;
        glmath::Vec3 d = glmath::normalise(lookat - pos);
        camera.yaw = atan2(d.z, d.x) - glmath::PI /2;
        float horizontalDist = sqrt(d.x * d.x + d.z * d.z);
        camera.pitch = atan2(d.y, horizontalDist);
    }

    void setBackgroundColour(nanobind::ndarray<f32, nanobind::shape<3>> np_colour)
    {
        glClearColor(np_colour(0), np_colour(1), np_colour(2),1.0);
    }
#else
    void particles(const std::vector<glmath::Vec3> &centres, const std::vector<glmath::Vec4> &colours)
    {
        i64 points_to_allocate = centres.size();
        for (i64 i = 0; i < points_to_allocate; ++i)
        {
            glmath::Vec4 pos = {centres[i].x, centres[i].y, centres[i].z, 0.0};
            glmath::Vec4 colour =colours[i];
            renderer.particle_data.push_back({pos, colour});
        }
    }

    void setCamera(glmath::Vec3 pos, glmath::Vec3 lookat)
    {
        camera.pos = pos;
        glmath::Vec3 d = glmath::normalise(lookat - pos);
        camera.yaw = atan2(d.z, d.x) - glmath::PI /2;
        float horizontalDist = sqrt(d.x * d.x + d.z * d.z);
        camera.pitch = atan2(d.y, horizontalDist);
    }
#endif




    void show()
    {
        constexpr f32 vertical_fov = 45.0 * glmath::PI / 180.0;
        constexpr f32 near_plane = 0.1f;
        constexpr f32 far_plane  = 1000.f;
        const f32 aspect_ratio = static_cast<f32>(surface_state.client_width) / static_cast<f32>(surface_state.client_height);
        glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);



        sortParticlesByDepth(renderer,camera.pos);
        renderScene(renderer,projection * camera.view);

        glXSwapBuffers(surface_state.connection, surface_state.window);


    }
    void logDiagnostics();
};


#if PYTHON_BINDING
NB_MODULE(glrendererX11, m) {
    nanobind::class_<GlRenderer>(m, "GlRenderer")
        .def(nanobind::init<>())
        .def("show", &GlRenderer::show)
        .def("windowIsClosed", &GlRenderer::windowIsClosed)
        .def("processWindowInput", &GlRenderer::processWindowInput)
        .def("logDiagnostics", &GlRenderer::logDiagnostics)
        // .def("inspect", &GlRenderer::inspect)
        .def("particles", &GlRenderer::particles)
        .def("setCamera", &GlRenderer::setCamera)
        .def("setBackgroundColour", &GlRenderer::setBackgroundColour);


}
#else


extern char _binary_spherePosNormalTriangulated_ply_start;
// extern char _binary_spherePosNormalTriangulated_ply_end;
// std::string_view loadSphere(StackArena &arena)
// {
//     const char *blob_start = &_binary_spherePosNormalTriangulated_ply_start;
//     const char *blob_end = &_binary_spherePosNormalTriangulated_ply_end;
//     // i64 size = std::bit_cast<intptr_t, char*>(blob_end) -std::bit_cast<intptr_t, char*>(blob_start);
//     // assert(size > 0 );
//     return {};
// }

int main()
{

        srand(20);
        
        i32 dim = 3;
        i32 n_points = 100000;
        std::vector<glmath::Vec3> points;
        std::vector<glmath::Vec4> colour;
        points.resize(n_points);
        colour.resize(n_points);

        for(i32 i = 0; i < n_points; ++i)
        {
            for(i32 j = 0; j < dim; ++j)
                points[i].data[j] = (static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX));

            float transparent = static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX);
            if(transparent < 0.5)
                colour[i] = {1.0, 0.0, 0.0, 1.0};
            else
                colour[i] = {0.0, 0.0, 1.0, 0.3};

        }   


    auto renderer = GlRenderer();

    glmath::Vec3 pos = {0.5,0.5,-2.0};
    glmath::Vec3 lookat = {0.5,0.5,0.5};


    renderer.setCamera(pos, lookat);

    while(!renderer.windowIsClosed())
    {
        // for(i32 i = 0; i < 100; ++i)
        // {
        //     points.push_back({static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX), static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX), static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX)});
        // }
  
        renderer.processWindowInput();
        renderer.particles(points, colour);
        renderer.show();
    }

    RENDERER_LOG("Exiting...");
}
#endif


void GlRenderer::logDiagnostics() {
    // Log X11 display information
    RENDERER_LOG("X11 Display Information:");
    RENDERER_LOG("  Display: %s", XDisplayName(nullptr));
    RENDERER_LOG("  Screen: %d (Default Screen: %d)", 
                 DefaultScreen(surface_state.connection), 
                 ScreenCount(surface_state.connection));
    RENDERER_LOG("  Root Window ID: %lu", RootWindow(surface_state.connection, DefaultScreen(surface_state.connection)));
    RENDERER_LOG("  Window ID: %lu", surface_state.window);
    RENDERER_LOG("  Client Width: %d", surface_state.client_width);
    RENDERER_LOG("  Client Height: %d", surface_state.client_height);

    // Log screen-specific information
    Screen* screen = ScreenOfDisplay(surface_state.connection, DefaultScreen(surface_state.connection));
    RENDERER_LOG("Screen Information:");
    RENDERER_LOG("  Width: %d", screen->width);
    RENDERER_LOG("  Height: %d", screen->height);
    RENDERER_LOG("  Depth: %d", screen->root_depth);


    // Log GLX information
    RENDERER_LOG("GLX Information:");
    int glxMajor, glxMinor;
    glXQueryVersion(surface_state.connection, &glxMajor, &glxMinor);
    RENDERER_LOG("  GLX Version: %d.%d", glxMajor, glxMinor);

    const char* glxExtensions = glXQueryExtensionsString(surface_state.connection, DefaultScreen(surface_state.connection));
    RENDERER_LOG("  GLX Extensions: %s", glxExtensions);

    // Log framebuffer configuration
    RENDERER_LOG("Framebuffer Configuration:");
    GLXFBConfig fbConfig = surface_state.fb_config;
    if (fbConfig) {
        int redBits, greenBits, blueBits, alphaBits, depthBits, stencilBits;
        glXGetFBConfigAttrib(surface_state.connection, fbConfig, GLX_RED_SIZE, &redBits);
        glXGetFBConfigAttrib(surface_state.connection, fbConfig, GLX_GREEN_SIZE, &greenBits);
        glXGetFBConfigAttrib(surface_state.connection, fbConfig, GLX_BLUE_SIZE, &blueBits);
        glXGetFBConfigAttrib(surface_state.connection, fbConfig, GLX_ALPHA_SIZE, &alphaBits);
        glXGetFBConfigAttrib(surface_state.connection, fbConfig, GLX_DEPTH_SIZE, &depthBits);
        glXGetFBConfigAttrib(surface_state.connection, fbConfig, GLX_STENCIL_SIZE, &stencilBits);
        RENDERER_LOG("  Red Bits: %d", redBits);
        RENDERER_LOG("  Green Bits: %d", greenBits);
        RENDERER_LOG("  Blue Bits: %d", blueBits);
        RENDERER_LOG("  Alpha Bits: %d", alphaBits);
        RENDERER_LOG("  Depth Bits: %d", depthBits);
        RENDERER_LOG("  Stencil Bits: %d", stencilBits);
    } else {
        RENDERER_LOG("  No framebuffer configuration available.");
    }

    // Log OpenGL context information
    RENDERER_LOG("OpenGL Context Information:");
    const u8* glVersion = glGetString(GL_VERSION);
    RENDERER_LOG("  OpenGL Version: %s", glVersion);
    const u8* glVendor = glGetString(GL_VENDOR);
    RENDERER_LOG("  Vendor: %s", glVendor);
    const u8* glRendererStr = glGetString(GL_RENDERER);
    RENDERER_LOG("  Renderer: %s", glRendererStr);
    const u8* glShadingLanguageVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    RENDERER_LOG("  GLSL Version: %s", glShadingLanguageVersion);

    // Log OpenGL extensions
    RENDERER_LOG("OpenGL Extensions:");
    const u8* glExtensions = glGetString(GL_EXTENSIONS);
    RENDERER_LOG("  Extensions: %s", glExtensions);

    // Log GLX context attributes
    RENDERER_LOG("GLX Context Attributes:");
    GLXContext context = glXGetCurrentContext();
    if (context) {
        int contextAttribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
            None
        };
        RENDERER_LOG("  Context Major Version: %d", contextAttribs[1]);
        RENDERER_LOG("  Context Minor Version: %d", contextAttribs[3]);
        RENDERER_LOG("  Context Profile: %s", 
                     (contextAttribs[5] == GLX_CONTEXT_CORE_PROFILE_BIT_ARB) ? "Core" : "Compatibility");
    } else {
        RENDERER_LOG("  No current GLX context.");
    }

    // Log visual info
    XVisualInfo* visualInfo = glXGetVisualFromFBConfig(surface_state.connection, fbConfig);
    if (visualInfo) {
        RENDERER_LOG("Visual Information:");
        RENDERER_LOG("  Visual ID: %lu", visualInfo->visualid);
        RENDERER_LOG("  Depth: %d", visualInfo->depth);
        RENDERER_LOG("  Class: %s", 
                      (visualInfo->c_class == StaticGray) ? "StaticGray" :
                      (visualInfo->c_class == GrayScale) ? "GrayScale" :
                      (visualInfo->c_class == StaticColor) ? "StaticColor" :
                      (visualInfo->c_class == PseudoColor) ? "PseudoColor" :
                      (visualInfo->c_class == TrueColor) ? "TrueColor" :
                      (visualInfo->c_class == DirectColor) ? "DirectColor" : "Unknown");
        XFree(visualInfo);
    } else {
        RENDERER_LOG("  No visual information available.");
    }

    // Log current OpenGL state
    RENDERER_LOG("OpenGL State:");
    i32 maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    RENDERER_LOG("  Max Texture Size: %d", maxTextureSize);

    i32 maxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
    RENDERER_LOG("  Max Viewport Dimensions: %d x %d", maxViewportDims[0], maxViewportDims[1]);

    i32 viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    RENDERER_LOG("  Current Viewport: %d, %d, %d, %d", viewport[0], viewport[1], viewport[2], viewport[3]);

    i32 scissorBox[4];
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
    RENDERER_LOG("  Scissor Box: %d, %d, %d, %d", scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);

    bool depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    RENDERER_LOG("  Depth Test Enabled: %s", depthTestEnabled ? "Yes" : "No");
    i32 depthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    RENDERER_LOG("  Depth Test Function: %d", depthFunc); 


    bool cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    RENDERER_LOG("  Cull Face Enabled: %s", cullFaceEnabled ? "Yes" : "No");

    i32 frontFace;
    glGetIntegerv(GL_FRONT_FACE, &frontFace);
    RENDERER_LOG("  Front Face: %s", frontFace == GL_CW ? "CW" : "CCW");
}