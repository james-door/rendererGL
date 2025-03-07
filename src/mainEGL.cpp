#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>

#include <EGL/egl.h>

#include "external/glad/glad.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include "defintions.h"
#include "glmath.h"
#include "renderer.cpp"


int main() {
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
    

    // Camera
    f32 vertical_fov = 45.0 * glmath::PI / 180.0;
    f32 near_plane = 0.1f;
    f32 far_plane  = 1000.f;
    f32 aspect_ratio = static_cast<f32>(client_width) / static_cast<f32>(client_height);
    glmath::Mat4x4 projection = glmath::perspectiveProjection(vertical_fov,aspect_ratio,near_plane,far_plane);
    glmath::Vec3 camera_pos = {0.0,0.0,-5.0};
    glmath::Vec3 look_at = {0.0,0.0,0.0};
    constexpr glmath::Vec3 up = {0.0,1.0,0.0};
    glmath::Mat4x4 view = glmath::lookAt(camera_pos,look_at,up);
    glmath::Mat4x4 mvp = projection * view;
    stbi_flip_vertically_on_write(true);
    std::vector<u8> colour_buffer(client_width * client_height * 3);



    //inital opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glCullFace(GL_FRONT);
    glClearColor(1.0,1.0,1.0,1.0);
    glViewport(0, 0, client_width, client_height);
    i32 n_points = 100;
    Renderer renderer = {};
    initialiseRenderer(renderer, n_points);
    srand(20);
    i32 dim = 3;
    for(auto &point : renderer.particle_data)
    {
        for(i32 i = 0; i < dim; ++i)
            point.position.data[i] = (static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX));
        
        float transparent = static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX);
        if(transparent < 0.5)
            point.colour = {1.0, 0.0, 0.0, 1.0};
        else
            point.colour = {0.0, 0.0, 1.0, transparent};

    }   
    renderer.light_pos[0] = {2.0};
    uploadParticleData(renderer);


    while(1)
    {
        renderer.debug_aabb[0] = {{0.0}, {1.0}};
        renderer.colour[MAX_DEBUG_LINES] = {0.0,1.0,0.0};
        
        renderScene(renderer,mvp,n_points);
        
        glReadPixels(0,0,client_width, client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
        stbi_write_png("test.png",client_width,client_height,3,colour_buffer.data(), client_width * 3);

        eglSwapBuffers(connection,offscreen_surface);
    }
}