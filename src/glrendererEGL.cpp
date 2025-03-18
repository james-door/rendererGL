// BUGS:
// 1. X11 Breaks most of the time when using debugpy
//TODO:
        
// 2. should be able to switch between double precision and single precesion in both C++ and glsl when compiling
// 3. imposter spheres and pre-depth pass
// 5. support for deubg rendering lines and AABB 
// 6. currently the radius will affect all particles drawn, there is no per particle nor per call radius
// 7. support for adding point lights

// FUTURE:
// 1. support for "Headless" x11 rendering
// 2. support for dyamic loading of opengl/egl or opengl/x11 in the same file


#if PYTHON_BINDING
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#endif

#include <string>
#include <filesystem>
#include <vector>

// #include <EGL/egl.h>
#include "external/glad/glad.h"
#include "external/glad/glad_egl.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include "defintions.h" 
#include "glmath.h"
#include "renderer.cpp"



struct SurfaceState
{
    EGLDisplay connection;    
    EGLSurface surface;
    i32 client_width;
    i32 client_height;
};



SurfaceState createSurfaceAndContext(i32 client_width, i32 client_height)
{
  static const EGLint attribute_list[] = {
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_DEPTH_SIZE, 8,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
          EGL_NONE
  };    
    EGLint offscreen_buffer_attributes[] = {EGL_HEIGHT, client_height, EGL_WIDTH, client_width,EGL_NONE};



    gladLoadEGL();
    eglBindAPI(EGL_OPENGL_API);

    static const int MAX_DEVICES = 20;
    EGLDeviceEXT devices[MAX_DEVICES];
    EGLint numDevices;
    eglQueryDevicesEXT(MAX_DEVICES, devices, &numDevices);

    
    RENDERER_LOG("Found %d device(s)\n", numDevices);
    for (int i = 0; i < numDevices; i++) {
        const char *vendor = eglQueryDeviceStringEXT(devices[i], EGL_VENDOR);
        const char *extensions = eglQueryDeviceStringEXT(devices[i], EGL_EXTENSIONS);
        RENDERER_LOG("Device %d vendor: %s\n", i, vendor ? vendor : "unknown");
        RENDERER_LOG("Device %d extensions: %s\n", i, extensions ? extensions : "unknown");
    }

    EGLDisplay connection = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[0], 0);

    // EGLDisplay connection = eglGetDisplay(eglDpy);

    RENDERER_ASSERT(connection != EGL_NO_DISPLAY, "EGL couldn't find any valid display connections.");

    EGLBoolean initalisationSuccess = eglInitialize(connection, NULL, NULL);
    RENDERER_ASSERT(initalisationSuccess == EGL_TRUE, "Couldn't initialise EGL.");

    EGLConfig config;
    i32 num_config;
    eglChooseConfig(connection, attribute_list, &config, 1, &num_config);
    RENDERER_ASSERT(num_config > 0, "Chosen display connection doesn't support rendering config.");

    EGLContext context = eglCreateContext(connection, config, EGL_NO_CONTEXT, NULL);
    RENDERER_ASSERT(context != EGL_NO_CONTEXT, "Couldn't crate EGL context.");

    EGLSurface offscreen_surface = eglCreatePbufferSurface(connection,config,offscreen_buffer_attributes);
    RENDERER_ASSERT(offscreen_surface != EGL_NO_SURFACE, "Can't create an offscreen surface.");

    
    EGLBoolean context_creation_success = eglMakeCurrent(connection, offscreen_surface, offscreen_surface, context);
    RENDERER_ASSERT(context_creation_success, "Coudn't make EGL context current.");

    i32 glad_load_success = gladLoadGLLoader((GLADloadproc)eglGetProcAddress);
    RENDERER_ASSERT(glad_load_success, "Couln't load EGL functions.");

    return {connection, offscreen_surface,client_width, client_height};
}


void renderTriangle()
{
    RENDERER_LOG("Current Working Directory: %s", std::filesystem::current_path().c_str());


    SurfaceState surface = createSurfaceAndContext(2000, 2000);

    StackArena triangle_arena{1024 * 10};
    auto vs = loadFile(triangle_arena, "src/shaders/triangleVS.glsl");
    auto fs = loadFile(triangle_arena, "src/shaders/triangleFS.glsl");

    auto vs_obj = compileShader(vs, GL_VERTEX_SHADER);
    auto fs_obj = compileShader(fs, GL_FRAGMENT_SHADER);
    RENDERER_ASSERT(vs_obj != -1 && fs_obj != -1, "Bad shader");

    u32 shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program,static_cast<u32>(vs_obj));
    glAttachShader(shader_program,static_cast<u32>(fs_obj));
    glLinkProgram(shader_program);
    i32 program_created;
    glGetProgramiv(shader_program,GL_LINK_STATUS,&program_created);
    RENDERER_ASSERT(program_created, "bad shader");

    glClearColor(1.0,0.0,0.0,1.0);
    glViewport(0, 0, 2000, 2000);


    glClear(GL_COLOR_BUFFER_BIT);

    u32 dummy_vao;
    glGenVertexArrays(1, &dummy_vao);
    glBindVertexArray(dummy_vao);

    glUseProgram(shader_program);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    std::vector<u8> colour_buffer(2000 * 2000 * 3);

    glReadPixels(0,0,2000, 2000, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png("test.png",2000, 2000 ,3, colour_buffer.data(), 2000 * 3);
}





struct Camera
{
    glmath::Vec3 pos;
    glmath::Vec3 lookat;
};


struct GlRenderer
{
    SurfaceState surface_state;
    Renderer renderer;
    Camera camera;
    GlRenderer(i32 width, i32 height)
    {
        RENDERER_LOG("Current Working Directory: %s", std::filesystem::current_path().c_str());
        const auto programStartTime = std::chrono::steady_clock::now();
        
        surface_state = createSurfaceAndContext(width, height);

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

        stbi_flip_vertically_on_write(true);

        camera = {.pos={0.0,0.0,0.0}, .lookat={0.0,0.0,1.0}};
        
        const auto reachRenderLoopTime = std::chrono::steady_clock::now();
        const std::chrono::duration<double> launchTime = reachRenderLoopTime - programStartTime;
        constexpr i32 titleBarMaxLength = 100;
        char titleBarString[titleBarMaxLength];
        snprintf(titleBarString,titleBarMaxLength,"Launch: %3fms",launchTime.count() * 1000.0);
        RENDERER_LOG(titleBarString);
    }



// #define PYTHON_BINDING 1
#if PYTHON_BINDING
    void particles(nanobind::ndarray<f32, nanobind::shape<-1, 3>>& centres, nanobind::ndarray<f32, nanobind::shape<-1, 4>>& colours, f32 radius)
    {
        RENDERER_ASSERT((centres.ndim() == 2), "Expected array to be dimension %d",2);
        RENDERER_ASSERT((colours.ndim() == 2), "Expected array to be dimension %d",2);
        setRadius(renderer,radius);

        
        // RENDERER_LOG("Device ID = %u (cpu=%i, cuda=%i)\n", a.device_id(),int(a.device_type() == nanobind::device::cpu::value),int(a.device_type() == nanobind::device::cuda::value));
        
        i64 points_to_allocate = centres.shape(0);
        for (i64 i = 0; i < points_to_allocate; ++i)
        {
            glmath::Vec4 pos = {centres(i, 0), centres(i, 1), centres(i,2), 0.0};
            glmath::Vec4 colour = {colours(i, 0), colours(i, 1), colours(i, 2), colours(i, 3)};
            // RENDERER_LOG("%f, %f, %f, %f",colour.x,colour.y, colour.z,colour.a);
            renderer.particle_data.push_back({pos, colour});
        }
    }

    void setCamera(nanobind::ndarray<f32, nanobind::shape<3>> np_pos, nanobind::ndarray<f32, nanobind::shape<3>> np_lookat)
    {
        glmath::Vec3 pos = {np_pos(0), np_pos(1), np_pos(2)};
        glmath::Vec3 lookat= {np_lookat(0), np_lookat(1), np_lookat(2)};
        camera.pos = pos;
        camera.lookat = lookat;
    }

    void setBackgroundColour(nanobind::ndarray<f32, nanobind::shape<3>> np_colour)
    {
        glClearColor(np_colour(0), np_colour(1), np_colour(2),1.0);
    }

    auto getImageRGB()
    {
        constexpr f32 vertical_fov = 45.0 * glmath::PI / 180.0;
        constexpr f32 near_plane = 0.1f;
        constexpr f32 far_plane  = 1000.f;
        const f32 aspect_ratio = static_cast<f32>(surface_state.client_width) / static_cast<f32>(surface_state.client_height);
        glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);
        
        constexpr glmath::Vec3 up = {0.0, 1.0, 0.0};
        glmath::Mat4x4 view = glmath::lookAt(camera.pos,camera.lookat,up);

        sortParticlesByDepth(renderer,camera.pos);
        renderScene(renderer, view, projection);
        
        // eglSwapBuffers(surface_state.connection, surface_state.surface);
        glFlush();


        i32 n_channels= 3;
        std::vector<u8> colour_buffer(surface_state.client_width * surface_state.client_height * n_channels);
        std::vector<u8> colour_buffer_flipped(surface_state.client_width * surface_state.client_height * n_channels);
        i32 pitch = surface_state.client_width * n_channels;

        glReadPixels(0,0,surface_state.client_width, surface_state.client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());

        for (i32 i = 0; i < surface_state.client_width * surface_state.client_height * n_channels; ++i) {
            i32 row = i / pitch;
            i32 col = i % pitch;
            colour_buffer_flipped[i] = colour_buffer[(surface_state.client_height - 1 - row) * pitch + col];
        }




        return nanobind::cast(nanobind::ndarray<u8, nanobind::numpy>(colour_buffer_flipped.data(), {static_cast<u64>(surface_state.client_height), static_cast<u64>(surface_state.client_width), static_cast<u64>(n_channels)}));
    }

    auto saveImageRGB(std::string path)
    {
        constexpr f32 vertical_fov = 45.0 * glmath::PI / 180.0;
        constexpr f32 near_plane = 0.1f;
        constexpr f32 far_plane  = 1000.f;
        const f32 aspect_ratio = static_cast<f32>(surface_state.client_width) / static_cast<f32>(surface_state.client_height);
        glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);
        constexpr glmath::Vec3 up = {0.0, 1.0, 0.0};
        glmath::Mat4x4 view = glmath::lookAt(camera.pos,camera.lookat,up);

        sortParticlesByDepth(renderer,camera.pos);
        renderScene(renderer, view, projection);
        
        // eglSwapBuffers(surface_state.connection, surface_state.surface);
        glFlush();

        i32 n_channels = 3;
        std::vector<u8> colour_buffer(surface_state.client_width * surface_state.client_height * n_channels);

        glReadPixels(0,0,surface_state.client_width, surface_state.client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
        stbi_write_png("test.png",surface_state.client_width, surface_state.client_height ,3, colour_buffer.data(), surface_state.client_width * 3);
    }

#else
    void particles(const std::vector<glmath::Vec3> &centres, const std::vector<glmath::Vec4> &colours, f32 radius)
    {
        // set the radius for all particles in the frame, should really just be for this call
        setRadius(renderer,radius);


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
        camera.lookat = lookat;
    }
    void setBackgroundColour(glmath::Vec3 colour)
    {
        glClearColor(colour.r, colour.g, colour.b, 1.0);
    }
    void show(const std::string &path)
    {
        constexpr f32 vertical_fov = 45.0 * glmath::PI / 180.0;
        constexpr f32 near_plane = 0.1f;
        constexpr f32 far_plane  = 1000.f;
        const f32 aspect_ratio = static_cast<f32>(surface_state.client_width) / static_cast<f32>(surface_state.client_height);
        glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);
        
        constexpr glmath::Vec3 up = {0.0, 1.0, 0.0};
        glmath::Mat4x4 view = glmath::lookAt(camera.pos,camera.lookat,up);

        sortParticlesByDepth(renderer,camera.pos);
        renderScene(renderer, view, projection);
        // eglSwapBuffers(surface_state.connection, surface_state.surface);
        glFinish();

        std::vector<u8> colour_buffer(surface_state.client_width * surface_state.client_height * 3);

        glReadPixels(0,0,surface_state.client_width, surface_state.client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
        stbi_write_png("test.png",surface_state.client_width, surface_state.client_height,3,colour_buffer.data(), surface_state.client_width * 3);
    }
#endif

    void logDiagnostics();
};


#if PYTHON_BINDING
NB_MODULE(glrendererEGL, m) {
    m.def("renderTriangle", &renderTriangle);
    // nanobind::class_<GlRenderer>(m, "GlRenderer")
    //     .def(nanobind::init<i32, i32>())
    //     .def("getImageRGB", &GlRenderer::getImageRGB)
    //     .def("particles", &GlRenderer::particles)
    //     .def("setCamera", &GlRenderer::setCamera)
    //     .def("setBackgroundColour", &GlRenderer::setBackgroundColour)
    //     .def("saveImageRGB", &GlRenderer::saveImageRGB);
}

#else










int main()
{

    renderTriangle();
    return 0;
    srand(20);
    
    i32 dim = 3;
    i32 n_points = 1000;
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
            colour[i] = {0.925, 0.329, 0.231, 1.0};
        else
            colour[i] = {0.023, 0.522, 0.490, 0.2};

    }   


    auto renderer = GlRenderer(2000, 2000);

    glmath::Vec3 pos = {0.5,0.5,-2.0};
    glmath::Vec3 lookat = {0.5,0.5,0.5};

    renderer.setCamera(pos, lookat);

    renderer.particles(points, colour, 0.01);
    renderer.show("test.png");



    RENDERER_LOG("Exiting...");
}
#endif
