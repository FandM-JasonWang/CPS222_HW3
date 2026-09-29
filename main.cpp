// Author: Jason Wang (Assisted by Gemini 3.1 Pro Extended and ChatGPT 5.6 Luna)
// Sources Cited:
// - X11 setup and 3D projection formulas derived from class slides & transcription.
// - Bresenham's line algorithm referenced from standard computer graphics principles.
// - Many stackoverflow discussions on 3D rendering and X11 graphics.
// - Some Reddit jokes about 3D shapes and rendering.

#include <X11/Xlib.h> // use wsl and please ignore this warning: "X11/Xlib.h: No such file or directory"
#include <X11/keysym.h> // use wsl and please ignore this warning: "X11/keysym.h: No such file or directory"
#include <X11/Xutil.h> // use wsl and please ignore this warning: "X11/Xutil.h: No such file or directory"
#include <cmath>
#include <vector>
#include <unistd.h> // use wsl and please ignore this warning: "unistd.h: No such file or directory"
#include <sys/time.h> // use wsl and please ignore this warning: "sys/time.h: No such file or directory"
#include <stdint.h>
#include <iostream>

const int WIDTH = 800;
const int HEIGHT = 600;
const double FOCAL_LENGTH = 400.0;

struct point_3d_t
{
    double x, y, z;
};

struct shape_3d_t
{
    std::vector<point_3d_t> pts;
    std::vector<std::pair<int, int>> edges;
};

// Time in microseconds for 60FPS calculation
long long current_time_micros()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

// Converts 3D coordinates into 2D flat screen coordinates
point_3d_t project(point_3d_t p, double cam_x, double cam_y, double cam_z, double cam_yaw)
{
    p.x -= cam_x;
    p.y -= cam_y;
    p.z -= cam_z;

    // Simulate head rotation around Y axis
    double cos_y = std::cos(-cam_yaw);
    double sin_y = std::sin(-cam_yaw);
    double rot_x = p.x * cos_y - p.z * sin_y;
    double rot_z = p.x * sin_y + p.z * cos_y;
    p.x = rot_x;
    p.z = rot_z;

    if (p.z <= 0.1)
        p.z = 0.1;

    point_3d_t screen;
    // Perspective projection
    screen.x = p.x * FOCAL_LENGTH / p.z;
    screen.y = p.y * FOCAL_LENGTH / p.z;

    // Invert Y and center to match screen coordinates (top-left origin)
    screen.x = screen.x + (WIDTH / 2.0);
    screen.y = -1.0 * screen.y + (HEIGHT / 2.0);
    screen.z = p.z;

    return screen;
}

// Bresenham's algorithm to write directly to buffer
void draw_line(uint32_t *buffer, int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) // the following loop draws the line pixel by pixel using Bresenham's algorithm
    {
        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT)
        {
            buffer[y0 * WIDTH + x0] = color;
        }
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

int main()
{
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        std::cerr << "Could not open X11 display!" << std::endl;
        return 1;
    }

    int screen = DefaultScreen(dpy);
    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, WIDTH, HEIGHT, 1, BlackPixel(dpy, screen), BlackPixel(dpy, screen));
    XSelectInput(dpy, win, ExposureMask | KeyPressMask);
    XStoreName(dpy, win, "3D Floating Shape");
    XMapWindow(dpy, win);
    GC gc = DefaultGC(dpy, screen);

    // Frame buffer to prevent screen flickering
    uint32_t *frame_buffer = new uint32_t[WIDTH * HEIGHT];
    XImage *image = XCreateImage(dpy, DefaultVisual(dpy, screen), 24, ZPixmap, 0, (char *)frame_buffer, WIDTH, HEIGHT, 32, 0);

    shape_3d_t cube;
    cube.pts = {{-10, 10, -10}, {10, 10, -10}, {10, 10, 10}, {-10, 10, 10}, {-10, -10, -10}, {10, -10, -10}, {10, -10, 10}, {-10, -10, 10}};
    cube.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 6}, {5, 7}}; // Added 'X' on bottom

    shape_3d_t pyramid;
    pyramid.pts = {{-10, -10, -10}, {10, -10, -10}, {10, -10, 10}, {-10, -10, 10}, {0, 10, 0}};
    pyramid.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}};

    shape_3d_t prism;
    prism.pts = {{-10, -10, -10}, {10, -10, -10}, {0, -10, 10}, {-10, 10, -10}, {10, 10, -10}, {0, 10, 10}};
    prism.edges = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}, {0, 3}, {1, 4}, {2, 5}};

    shape_3d_t stickman;
    stickman.pts = {
        {0, 14, 0}, {3, 11, 0}, {-3, 11, 0}, {0, 11, 3}, {0, 11, -3}, {0, 8, 0}, {0, -2, 0}, {-12, 10, 2}, {12, 10, -2}, {-7, -16, 3}, {7, -16, -3}};
    stickman.edges = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 3}, {3, 2}, {2, 4}, {4, 1}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 6}, {5, 7}, {5, 8}, {6, 9}, {6, 10}};

    shape_3d_t ufo;
    ufo.pts = {
        {0, 8, 0},
        {6, 3, 0},
        {3, 3, 5.2},
        {-3, 3, 5.2},
        {-6, 3, 0},
        {-3, 3, -5.2},
        {3, 3, -5.2},
        {14, 0, 0},
        {7, 0, 12.1},
        {-7, 0, 12.1},
        {-14, 0, 0},
        {-7, 0, -12.1},
        {7, 0, -12.1},
        {0, -5, 0}};
    ufo.edges = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 1}, {1, 7}, {2, 8}, {3, 9}, {4, 10}, {5, 11}, {6, 12}, {7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 12}, {12, 7}, {7, 13}, {8, 13}, {9, 13}, {10, 13}, {11, 13}, {12, 13}};

    shape_3d_t swirl;
    swirl.pts = {
        {-12, -10, -12}, {12, -10, -12}, {12, -10, 12}, {-12, -10, 12}, {0, -2, -10}, {10, -2, 0}, {0, -2, 10}, {-10, -2, 0}, {-5, 5, -5}, {5, 5, -5}, {5, 5, 5}, {-5, 5, 5}, {3, 12, 2}};
    swirl.edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {0, 7}, {1, 4}, {2, 5}, {3, 6}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {4, 8}, {5, 9}, {6, 10}, {7, 11}, {8, 9}, {9, 10}, {10, 11}, {11, 8}, {8, 12}, {9, 12}, {10, 12}, {11, 12}};

    shape_3d_t hourglass;
    hourglass.pts = {
        {-10, 12, -10}, {10, 12, -10}, {10, 12, 10}, {-10, 12, 10}, {-2, 0, -2}, {2, 0, -2}, {2, 0, 2}, {-2, 0, 2}, {-10, -12, -10}, {10, -12, -10}, {10, -12, 10}, {-10, -12, 10}};
    hourglass.edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 2}, {1, 3}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {4, 8}, {5, 9}, {6, 10}, {7, 11}, {8, 9}, {9, 10}, {10, 11}, {11, 8}, {8, 10}, {9, 11}};

    shape_3d_t airplane;
    airplane.pts = {
        {0, 0, 16},
        {-16, 4, -12},
        {16, 4, -12},
        {0, 2, -10},
        {0, -6, -6},
        {0, -4, -12}};
    airplane.edges = {
        {0, 1}, {1, 3}, {0, 2}, {2, 3}, {0, 3}, {0, 4}, {4, 5}, {5, 3}, {4, 3}, {1, 4}, {2, 4}};

    std::vector<shape_3d_t> shapes = {cube, pyramid, prism, stickman, ufo, swirl, hourglass, airplane};
    int current_shape = 0;
    bool show_grid = true;

    double cam_x = 0, cam_y = 0, cam_z = -50.0;
    double cam_yaw = 0;

    bool running = true;
    long long last_time = current_time_micros();

    while (running)
    {
        while (XPending(dpy) > 0)
        {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.type == KeyPress) // the following ifs handle key presses for camera movement, shape selection, and grid toggling
            {
                KeySym key = XLookupKeysym(&ev.xkey, 0);
                if (key == XK_Escape)
                    running = false;
                if (key == XK_1)
                    current_shape = 0;
                if (key == XK_2)
                    current_shape = 1;
                if (key == XK_3)
                    current_shape = 2;
                if (key == XK_4)
                    current_shape = 3;
                if (key == XK_5)
                    current_shape = 4;
                if (key == XK_6)
                    current_shape = 5;
                if (key == XK_7)
                    current_shape = 6;
                if (key == XK_8)
                    current_shape = 7;
                if (key == XK_g)
                    show_grid = !show_grid;
                if (key == XK_Left)
                    cam_x -= 2.0;
                if (key == XK_Right)
                    cam_x += 2.0;
                if (key == XK_a)
                    cam_yaw -= 0.05;
                if (key == XK_d)
                    cam_yaw += 0.05;
            }
        }

        for (int i = 0; i < WIDTH * HEIGHT; ++i)
            frame_buffer[i] = 0x000000;

        if (show_grid)
        {
            uint32_t grid_color = 0x004400;
            for (int i = -100; i <= 100; i += 20)
            {
                point_3d_t p1 = project({(double)i, -20.0, -100.0}, cam_x, cam_y, cam_z, cam_yaw); // Project the first point of the line
                point_3d_t p2 = project({(double)i, -20.0, 100.0}, cam_x, cam_y, cam_z, cam_yaw);  // Project the second point of the line
                draw_line(frame_buffer, p1.x, p1.y, p2.x, p2.y, grid_color);                       // Draw the line between the two projected points

                point_3d_t p3 = project({-100.0, -20.0, (double)i}, cam_x, cam_y, cam_z, cam_yaw); // Project the third point of the line
                point_3d_t p4 = project({100.0, -20.0, (double)i}, cam_x, cam_y, cam_z, cam_yaw);  // Project the fourth point of the line
                draw_line(frame_buffer, p3.x, p3.y, p4.x, p4.y, grid_color);                       // Draw the line between the two projected points
            }
        }

        double angle_rads = current_time_micros() / 1000000.0;
        shape_3d_t active = shapes[current_shape];
        std::vector<point_3d_t> projected_pts;

        for (const auto &pt : active.pts) // loop shows the rotation of the shape around the Y-axis
        {
            point_3d_t rot_pt;
            rot_pt.x = pt.x * std::cos(angle_rads) - pt.z * std::sin(angle_rads);
            rot_pt.y = pt.y;
            rot_pt.z = pt.x * std::sin(angle_rads) + pt.z * std::cos(angle_rads);
            projected_pts.push_back(project(rot_pt, cam_x, cam_y, cam_z, cam_yaw));
        }

        uint32_t shape_color = 0xFF0000;
        for (const auto &edge : active.edges) // first = index of first point, second = index of second point
        {
            point_3d_t p1 = projected_pts[edge.first];
            point_3d_t p2 = projected_pts[edge.second];
            draw_line(frame_buffer, p1.x, p1.y, p2.x, p2.y, shape_color);
        }

        XPutImage(dpy, win, gc, image, 0, 0, 0, 0, WIDTH, HEIGHT);
        XFlush(dpy);

        // Limit to 60 FPS (~16.6ms) to save energy
        long long elapsed = current_time_micros() - last_time;
        long long sleep_time = 16666 - elapsed;
        if (sleep_time > 0)
            usleep(sleep_time);
        last_time = current_time_micros();
    }

    image->data = nullptr;
    XDestroyImage(image);
    delete[] frame_buffer;
    XCloseDisplay(dpy);
    return 0;
}