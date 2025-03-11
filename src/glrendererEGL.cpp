// BUGS:
// 1. Breaks most of the time when using debugpy


//TODO:
// 1. support for deubg rendering lines and AABB 
// 2. support for adding point lights
// 3. fix debugpy issue
// 4. support for dyamic loading of opengl/egl or opengl/x11 in the same file
// 5. supprot for "Headless" x11 rendering



#if PYTHON_BINDING
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#endif


#include <string>
#include <filesystem>


#include <EGL/egl.h>
#include "external/glad/glad.h"
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



SurfaceState createSurfaceAndContext()
{
    constexpr i32 client_width = 1024;
    constexpr i32 client_height = 1024;
    constexpr EGLint attribute_list[] = {
            EGL_RED_SIZE, 1,
            EGL_GREEN_SIZE, 1,
            EGL_BLUE_SIZE, 1,
            EGL_NONE
    };
    constexpr EGLint offscreen_buffer_attributes[] = {EGL_HEIGHT, client_height, EGL_WIDTH, client_width,EGL_NONE};

    eglBindAPI(EGL_OPENGL_API);

    EGLDisplay connection = eglGetDisplay(EGL_DEFAULT_DISPLAY);
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
    GlRenderer()
    {
        RENDERER_LOG("Current Working Directory: %s", std::filesystem::current_path().c_str());
        const auto programStartTime = std::chrono::steady_clock::now();
        
        surface_state = createSurfaceAndContext();

        i32 gl_load_status = gladLoadGLLoader((GLADloadproc)eglGetProcAddress);


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

        stbi_flip_vertically_on_write(true);

        
        const auto reachRenderLoopTime = std::chrono::steady_clock::now();
        const std::chrono::duration<double> launchTime = reachRenderLoopTime - programStartTime;
        constexpr i32 titleBarMaxLength = 100;
        char titleBarString[titleBarMaxLength];
        snprintf(titleBarString,titleBarMaxLength,"Launch: %3fms",launchTime.count() * 1000.0);
        RENDERER_LOG(titleBarString);
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
        camera.lookat = lookat;
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
        camera.lookat = lookat;
    }
    void setBackgroundColour(glmath::Vec3 colour)
    {
        glClearColor(colour.r, colour.g, colour.b, 1.0);
    }
#endif

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
        renderScene(renderer,projection * view);
        eglSwapBuffers(surface_state.connection, surface_state.surface);
        std::vector<u8> colour_buffer(surface_state.client_width * surface_state.client_height * 3);

        glReadPixels(0,0,surface_state.client_width, surface_state.client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
        stbi_write_png("test.png",surface_state.client_width, surface_state.client_height,3,colour_buffer.data(), surface_state.client_width * 3);
    }
    void logDiagnostics();
};


#if PYTHON_BINDING
NB_MODULE(glrendererEGL, m) {
    nanobind::class_<GlRenderer>(m, "GlRenderer")
        .def(nanobind::init<>())
        .def("show", &GlRenderer::show)
        .def("particles", &GlRenderer::particles)
        .def("setCamera", &GlRenderer::setCamera)
        .def("setBackgroundColour", &GlRenderer::setBackgroundColour);


}
#else
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

        // for(i32 i = 0; i < 100; ++i)
        // {
        //     points.push_back({static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX), static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX), static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX)});
        // }
  
    renderer.particles(points, colour);
    renderer.show("test.png");



    RENDERER_LOG("Exiting...");
}
#endif