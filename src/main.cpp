#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <iostream>
#include "defintions.h"

int main() {
    Display* connection = XOpenDisplay(NULL);
    i32 default_screen = DefaultScreen(connection);
    Window root_window = RootWindow(connection,default_screen);

    i32 tl_window_x = 200;
    i32 tl_window_y = 200;
    i32 client_width = 1024;
    i32 client_height = 1024;
    i32 border_width = 4;


    int n_visuals;
    XVisualInfo* visual_info = XGetVisualInfo(connection, VisualNoMask, NULL, &n_visuals);

    Visual* visual = visual_info[0].visual;
    i32 bit_depth = visual_info[0].depth; // True colour

    u64 attirubte_mask = CWEventMask | CWColormap;
    XSetWindowAttributes window_attributes;
    window_attributes.colormap = XCreateColormap(connection,root_window,visual,AllocNone);
    window_attributes.event_mask = ExposureMask | KeyPressMask;


    Window window = XCreateWindow(connection,root_window,tl_window_x, tl_window_y, client_width, client_height, border_width, bit_depth,InputOutput,visual,attirubte_mask,&window_attributes);

    XSelectInput(connection,window,ExposureMask | KeyPressMask);

    XMapWindow(connection,window);
    
    XFlush(connection);

    XSetWindowBackground(connection, window, 0x00FF00);
    XEvent event;
    while(1)
    {
        if(XPending(connection) > 0)
            XNextEvent(connection,&event);
        
        // if(event.type == Expose)
        

    }


}
