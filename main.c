#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
int WINDOW_WIDTH = 512;
int WINDOW_HEIGHT = 512;

/* canvas size */
int WIDTH = 24;
int HEIGHT = 16;
/*                        h   w */
unsigned char canvas_data[16][24][4];

unsigned char palette_index = 0;
unsigned char palette_count = 8;
unsigned char palette[][8] = {
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

unsigned char zoom_level = 0;
unsigned char zoom[6] = {1, 2, 4, 8, 16, 32};

unsigned char pan_mode = 0;
signed char draw_mode = 1;
unsigned char *draw_color;
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
main(int argc, char **argv)
{
    GLFWwindow *window;

    draw_color = palette[palette_index];

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);

    window =
        glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "pixel-tool", NULL, NULL);

    if (!window)
        puts("Failed to create GLFW window");

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        puts("Failed to init GLAD");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    viewport[0] = WINDOW_WIDTH / 2 - WIDTH * zoom[zoom_level] / 2;
    viewport[1] = WINDOW_HEIGHT / 2 - HEIGHT * zoom[zoom_level] / 2;
    viewport[2] = WIDTH * zoom[zoom_level];
    viewport[3] = HEIGHT * zoom[zoom_level];

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
        WIDTH,
        HEIGHT,
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
            WIDTH,
            HEIGHT,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            canvas_data);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        if (should_save > 0) {
            unsigned char *data = (unsigned char *)malloc(
                WIDTH * HEIGHT * 4 * sizeof(unsigned char));

            printf(
                "allocated %lu. stride: %lu\n",
                WIDTH * HEIGHT * 4 * sizeof(unsigned char),
                sizeof(unsigned char) * 4);

            /* glPixelStorei(GL_UNPACK_ALIGNMENT, 1); */
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_write_png(FILE_NAME, WIDTH, HEIGHT, 4, data, 0);

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

        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            if (draw_mode > 0) {
                canvas_data[y][x][0] = draw_color[0];
                canvas_data[y][x][1] = draw_color[1];
                canvas_data[y][x][2] = draw_color[2];
                canvas_data[y][x][3] = draw_color[3];
            } else {
                canvas_data[y][x][0] = 0;
                canvas_data[y][x][1] = 0;
                canvas_data[y][x][2] = 0;
                canvas_data[y][x][3] = 0;
            }
        }
    }

    pan_mode = 0;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS &&
        (x < 0 || x > WIDTH) && (y < 0 || y > HEIGHT)) {
        pan_mode = 1;
    }
}

void
mouse_callback(GLFWwindow *window, double x, double y)
{
    /* infitesimally small chance aside from startup */
    if (cursor_pos_last[0] != 0 && cursor_pos_last[1] != 0) {
        if (pan_mode > 0) {
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
    cursor_pos_relative[1] = (y + viewport[1]) - (WINDOW_HEIGHT - viewport[3]);
}

void
mouse_button_callback(GLFWwindow *window, int button, int down, int c)
{
    if (button == GLFW_MOUSE_BUTTON_3 && down == GLFW_TRUE)
        draw_mode = -draw_mode;
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
        if (palette_index > 0)
            palette_index--;
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS)
        if (palette_index < palette_count - 1)
            palette_index++;

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        if (palette_count >= 1)
            palette_index = 0;

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        if (palette_count >= 2)
            palette_index = 1;

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        if (palette_count >= 3)
            palette_index = 2;

    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
        if (palette_count >= 4)
            palette_index = 3;

    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
        if (palette_count >= 5)
            palette_index = 4;

    if (key == GLFW_KEY_6 && action == GLFW_PRESS)
        if (palette_count >= 6)
            palette_index = 5;

    if (key == GLFW_KEY_7 && action == GLFW_PRESS)
        if (palette_count >= 7)
            palette_index = 6;

    if (key == GLFW_KEY_8 && action == GLFW_PRESS)
        if (palette_count >= 8)
            palette_index = 7;

    if (key == GLFW_KEY_9 && action == GLFW_PRESS)
        if (palette_count >= 9)
            palette_index = 8;

    if (key == GLFW_KEY_0 && action == GLFW_PRESS)
        if (palette_count >= 0)
            draw_mode = -1;

    draw_color = palette[palette_index];

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

    viewport[0] = WINDOW_WIDTH / 2 - WIDTH * zoom[zoom_level] / 2;
    viewport[1] = WINDOW_HEIGHT / 2 - HEIGHT * zoom[zoom_level] / 2;
    viewport[2] = WIDTH * zoom[zoom_level];
    viewport[3] = HEIGHT * zoom[zoom_level];

    viewport_set();
}
