#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "shader.h"
#include "math_linear.h"

/* CONFIGURATION BEGIN */
/* if you change from .png, it will still be a png */
char *FILE_NAME = "test.png";
int WINDOW_DIMS[2];
int DIMS[2];

unsigned char *canvas_data;

unsigned char palette_index = 1;
unsigned char palette_count = 8;
unsigned char palette[][4] = {
    {0, 0, 0, 0}, /* Clear Colour */
    {43, 15, 84, 255},
    {171, 31, 101, 255},
    {255, 79, 105, 255},
    {255, 247, 248, 255},
    {255, 129, 66, 255},
    {255, 218, 69, 255},
    {51, 104, 220, 255},
    {73, 231, 236, 255},
};

/* CONFIGURATION END */

enum mode {
    DRAW,
    PAN,
    FILL
};

unsigned char zoom_level = 0;
unsigned char zoom[6] = {1, 2, 4, 8, 16, 32};

enum mode mode = DRAW;
enum mode last_mode = DRAW;
unsigned char *draw_colour;
unsigned char erase[4] = { 0, 0, 0, 0 };
unsigned char should_save = 0;

GLfloat viewport[4];

/* clang-format off */
GLfloat vertices[] = {
     1.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
     1.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,
    -1.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f
};
/* clang-format on */
GLuint indices[] = {0, 1, 3, 1, 2, 3};

double cursor_pos[2];
double cursor_pos_last[2];
double cursor_pos_relative[2];

void
framebuffer_size_callback(GLFWwindow *, int, int);

void
process_input(GLFWwindow *);

void
mouse_callback(GLFWwindow *, double, double);

void
mouse_button_callback(GLFWwindow *, int, int, int);

void
scroll_callback(GLFWwindow *, double, double);

void
key_callback(GLFWwindow *, int, int, int, int);

void
viewport_set();

void
adjust_zoom(int);

int
string_to_int(int *out, char *s);

int
colour_equal(unsigned char*, unsigned char*);

void
fill(int x, int y, unsigned char *old_colour);

int
main(int argc, char **argv)
{
    WINDOW_DIMS[0] = 512;
    WINDOW_DIMS[1] = 512;

    DIMS[0] = 16;
    DIMS[1] = 16;
    
    unsigned char i;
    for (i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
        if (strcmp(argv[i], "-d") == 0) {
            int w, h;
            string_to_int(&w, argv[i+1]);
            string_to_int(&h, argv[i+2]);
            printf("%d %d\n", w, h);
            DIMS[0] = w;
            DIMS[1] = h;
            i += 2;
        }
    }

    canvas_data = (unsigned char*)malloc(DIMS[0] * DIMS[1] * 4 * sizeof(unsigned char));

    GLFWwindow *window;
    GLFWcursor* cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    draw_colour = palette[palette_index];

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);

    window =
        glfwCreateWindow(WINDOW_DIMS[0], WINDOW_DIMS[1], "pixel-tool", NULL, NULL);

    if (!window)
        puts("Failed to create GLFW window");

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        puts("Failed to init GLAD");

    glfwSetCursor(window, cursor);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    viewport[0] = WINDOW_DIMS[0] / 2 - DIMS[0] * zoom[zoom_level] / 2;
    viewport[1] = WINDOW_DIMS[1] / 2 - DIMS[1] * zoom[zoom_level] / 2;
    viewport[2] = DIMS[0] * zoom[zoom_level];
    viewport[3] = DIMS[1] * zoom[zoom_level];

    viewport_set();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    GLuint shader_program =
        create_shader_program("shader.vs", "shader.fs", NULL);

    GLuint vbo, vao, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        DIMS[0],
        DIMS[1],
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        canvas_data);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        process_input(window);

        glClearColor(0.075, 0.075, 0.1, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vao);

        glBindTexture(GL_TEXTURE_2D, 0);

        GLuint alpha_loc = glGetUniformLocation(shader_program, "alpha");
        glUniform1f(alpha_loc, 0.2f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glUniform1f(alpha_loc, 1.0f);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            DIMS[0],
            DIMS[1],
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            canvas_data);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        if (should_save > 0) {
            unsigned char *data = (unsigned char *)malloc(
                DIMS[0] * DIMS[1] * 4 * sizeof(unsigned char));

            printf(
                "allocated %lu. stride: %lu\n",
                DIMS[0] * DIMS[1] * 4 * sizeof(unsigned char),
                sizeof(unsigned char) * 4);

            /* glPixelStorei(GL_UNPACK_ALIGNMENT, 1); */
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_write_png(FILE_NAME, DIMS[0], DIMS[1], 4, data, 0);

            free(data);
            should_save = 0;
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void
framebuffer_size_callback(GLFWwindow *window, int w, int h)
{
    glViewport(0, 0, w, h);
}

void
process_input(GLFWwindow *window)
{
    int x, y;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        x = (int)(cursor_pos_relative[0] / zoom[zoom_level]);
        y = (int)(cursor_pos_relative[1] / zoom[zoom_level]);

        if (x >= 0 && x < DIMS[0] && y >= 0 && y < DIMS[1]) {
            unsigned char *ptr = canvas_data;
            ptr += (y * DIMS[0] + x) * 4;

            switch (mode) {
            case DRAW:
                *ptr = draw_colour[0];
                *(ptr + 1) = draw_colour[1];
                *(ptr + 2) = draw_colour[2];
                *(ptr + 3) = draw_colour[3];
                break;
            case PAN:
                break;
            case FILL:
                {
                unsigned char colour[4] = {*(ptr + 0),
                                           *(ptr + 1),
                                           *(ptr + 2),
                                           *(ptr + 3)};

                fill(x, y, colour);

                break;
                }
            }
        }
    }
}


void
mouse_callback(GLFWwindow *window, double x, double y)
{
    /* infitesimally small chance aside from startup */
    if (cursor_pos_last[0] != 0 && cursor_pos_last[1] != 0) {
        if (mode == PAN) {
            float xmov = (cursor_pos_last[0] - cursor_pos[0]);
            float ymov = (cursor_pos_last[1] - cursor_pos[1]);
            viewport[0] -= xmov;
            viewport[1] += ymov;
            viewport_set();
        }
    }

    cursor_pos_last[0] = cursor_pos[0];
    cursor_pos_last[1] = cursor_pos[1];
    cursor_pos[0] = x;
    cursor_pos[1] = y;
    cursor_pos_relative[0] = x - viewport[0];
    cursor_pos_relative[1] = (y + viewport[1]) - (WINDOW_DIMS[1] - viewport[3]);
}

void
mouse_button_callback(GLFWwindow *window, int button, int down, int c)
{

    /* if (button == GLFW_MOUSE_BUTTON_3 && down == GLFW_TRUE) */
    /*     draw_mode = -draw_mode; */

}

void
scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (yoffset > 0)
        adjust_zoom(1);
    else
        adjust_zoom(0);
}

void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_J && action == GLFW_PRESS) {
        if (palette_index > 1)
            palette_index--;
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS)
        if (palette_index < palette_count - 1)
            palette_index++;

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        if (palette_count >= 1)
            palette_index = 1;

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        if (palette_count >= 2)
            palette_index = 2;

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        if (palette_count >= 3)
            palette_index = 3;

    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
        if (palette_count >= 4)
            palette_index = 4;

    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
        if (palette_count >= 5)
            palette_index = 5;

    if (key == GLFW_KEY_6 && action == GLFW_PRESS)
        if (palette_count >= 6)
            palette_index = 6;

    if (key == GLFW_KEY_7 && action == GLFW_PRESS)
        if (palette_count >= 7)
            palette_index = 7;

    if (key == GLFW_KEY_8 && action == GLFW_PRESS)
        if (palette_count >= 8)
            palette_index = 8;

    if (key == GLFW_KEY_9 && action == GLFW_PRESS)
        if (palette_count >= 9)
            palette_index = 9;

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        mode = FILL;

    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        mode = DRAW;

    if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) {
            last_mode = mode;
            mode = PAN;
        } else if (action == GLFW_RELEASE) {
            mode = last_mode;
        }
    }

    if ((key == GLFW_KEY_0 || key == GLFW_KEY_E) && action == GLFW_PRESS) {
      palette_index = 0;
    }

    draw_colour = palette[palette_index];

    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        should_save = 1;
    }
}

void
viewport_set()
{
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void
adjust_zoom(int increase)
{
    if (increase > 0) {
        if (zoom_level < 5)
            zoom_level++;
    } else {
        if (zoom_level > 0)
            zoom_level--;
    }

    viewport[0] = WINDOW_DIMS[0] / 2 - DIMS[0] * zoom[zoom_level] / 2;
    viewport[1] = WINDOW_DIMS[1] / 2 - DIMS[1] * zoom[zoom_level] / 2;
    viewport[2] = DIMS[0] * zoom[zoom_level];
    viewport[3] = DIMS[1] * zoom[zoom_level];

    viewport_set();
}

int
string_to_int(int *out, char *s) {
    char *end;
    if (s[0] == '\0')
        return -1;
    long l = strtol(s, &end, 10);
    if (l > INT_MAX)
        return -2;
    if (l < INT_MIN)
        return -3;
    if (*end != '\0')
        return -1;
    *out = l;
    return 0;
}

int
colour_equal(unsigned char* a, unsigned char* b)
{
  printf("Checking colours: %d %d %d %d vs %d %d %d %d\n",
         *a, *(a+1), *(a+2), *(a+3),
         *b, *(b+1), *(b+2), *(b+3));
  if (
      *(a+0) == *(b+0) &&
      *(a+1) == *(b+1) &&
      *(a+2) == *(b+2) &&
      *(a+3) == *(b+3)) {
    return 1;
  }
  return 0;
}

unsigned char*
get_pixel(int x, int y)
{
    return canvas_data + ((y * DIMS[0] + x) * 4);   
}

void
fill(int x, int y, unsigned char *old_colour)
{
    unsigned char *ptr = get_pixel(x, y);
    if (colour_equal(ptr, old_colour))
    {
        *ptr = draw_colour[0];
        *(ptr + 1) = draw_colour[1];
        *(ptr + 2) = draw_colour[2];
        *(ptr + 3) = draw_colour[3];
        printf(">%d %d\n", x, y);

        if (x != 0 && !colour_equal(get_pixel(x - 1, y), draw_colour))
          fill(x - 1, y, old_colour);
        if (x != DIMS[0] - 1 && !colour_equal(get_pixel(x + 1, y), draw_colour))
          fill(x + 1, y, old_colour);
        if (y != DIMS[1] - 1 && !colour_equal(get_pixel(x, y + 1), draw_colour))
          fill(x, y + 1, old_colour);
        if (y != 0 && !colour_equal(get_pixel(x, y - 1), draw_colour))
          fill(x, y - 1, old_colour);
    }
}
