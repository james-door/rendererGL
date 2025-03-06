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

    
    EGLSurface offscreen_surface = eglCreatePbufferSurface(display,config,offscreen_buffer_attributes);
    assert(offscreen_surface != EGL_NO_SURFACE);

    EGLBoolean context_creation_success = eglMakeCurrent(display, offscreen_surface, offscreen_surface, context);
    assert(context_creation_success);
    

    i32 glad_load_success = gladLoadGLLoader((GLADloadproc)eglGetProcAddress);
    assert(glad_load_success);
    


    Renderer renderer = {};
    i32 n_points = 100;
    initialiseRenderer(renderer, n_points);



    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClearColor(1.0,1.0,1.0,1.0);
    glViewport(0, 0, client_width, client_height);
    

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

    while(1)
    {
        renderer.debug_aabb[0] = {{0.0}, {1.0}};
        renderer.colour[MAX_DEBUG_LINES] = {0.0,1.0,0.0};
        renderScene(renderer,mvp,n_points);
        
        glReadPixels(0,0,client_width, client_height, GL_RGB, GL_UNSIGNED_BYTE, colour_buffer.data());
        stbi_write_png("test.png",client_width,client_height,3,colour_buffer.data(), client_width * 3);

        eglSwapBuffers(display,offscreen_surface);
    }
}