#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include "shader.h"
#include "math_linear.h"

int WIDTH = 512;
int HEIGHT = 512;
unsigned char canvas_data[512][512][3];

int palette[] = {
    0xffffff, 0x000000, 0xff6666, 0xaaaa00
};

int PALETTE_WIDTH = 8 * 4;
int PALETTE_HEIGHT = 8;
unsigned char palette_data[4][3];

unsigned char zoom_level = 4;
unsigned char zoom_multiplier = 1;
float zoom = 1.0f;

unsigned char pan_mode = 0;

/* clang-format off */
GLfloat vertices[] = {
     1.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
     1.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,
    -1.0f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f
};

GLfloat palette_vertices[] = {
     1.0f,  1.0f,
     1.0f, -1.0f,
    -1.0f, -1.0f,
    -1.0f,  1.0f 
};
/* clang-format on */
GLuint indices[] = {0, 1, 3, 1, 2, 3};

GLfloat tex_coord_offset[2] = { 0.0f, 0.0f };

double cursor_pos[2];
double cursor_pos_last[2];

void
hex_to_rgb(int *hex, char *rgb)
{
    rgb[0] = (*hex >> 16) & 0xff;
    rgb[1] = (*hex >> 8) & 0xff;
    rgb[2] = (*hex >> 0) & 0xff;
}

void
adjust_zoom(int increase)
{
    if (increase > 0) {
        if (zoom_level < 5)
            zoom_level++;
        else
            puts("zoom maxed");
    } else {
        if (zoom_level > 0)
            zoom_level--;
        else
          puts("zoom at actual size");
    }

    switch (zoom_level) {
    case 0:
        zoom = 1.0f;
        tex_coord_offset[0] = 0.0f;
        tex_coord_offset[1] = 0.0f;
        zoom_multiplier = 1;
        break;
    case 1:
        zoom = 0.5f;
        tex_coord_offset[0] = 0.25f;
        tex_coord_offset[1] = 0.25f;
        zoom_multiplier = 2;
        break;
    case 2:
        zoom = 0.25f;
        tex_coord_offset[0] = 0.375f;
        tex_coord_offset[1] = 0.375f;
        zoom_multiplier = 4;
        break;
    case 3:
        zoom = 0.125f;
        tex_coord_offset[0] = 0.4375f;
        tex_coord_offset[1] = 0.4375f;
        zoom_multiplier = 8;
        break;
    case 4:
        zoom = 0.0625f;
        tex_coord_offset[0] = 0.4685f;
        tex_coord_offset[1] = 0.4685f;
        zoom_multiplier = 16;
    case 5:
        zoom = 0.03125f;
        tex_coord_offset[0] = 0.48525f;
        tex_coord_offset[1] = 0.48525f;
        zoom_multiplier = 32;
    }
}

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

int
main(int argc, char **argv)
{
    GLFWwindow *window;
    /* GLFWcursor *cursor; */

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "pixel-tool", NULL, NULL);

    if (!window)
        puts("Failed to create GLFW window");

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        puts("Failed to init GLAD");

    glViewport(0, 0, WIDTH, HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    /* cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR); */
    /* glfwSetCursor(window, cursor); */

    adjust_zoom(1);

    /* ? */
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, canvas_data);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int palette_shader_program = create_shader_program("palette.vs", "palette.fs", NULL);

    unsigned int palette_vbo, palette_vao, palette_ebo;
    glGenBuffers(1, &palette_vbo);
    glGenBuffers(1, &palette_ebo);
    glGenVertexArrays(1, &palette_vao);

    glBindVertexArray(palette_vao);

    glBindBuffer(GL_ARRAY_BUFFER, palette_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(palette_vertices), palette_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        process_input(window);

        glClearColor(0.0, 0.0, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        GLuint zoom_location = glGetUniformLocation(shader_program, "zoom");
        glUniform1f(zoom_location, zoom);

        GLuint offset_location = glGetUniformLocation(shader_program, "offset");
        glUniform2f(offset_location, tex_coord_offset[0], tex_coord_offset[1]);

        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, canvas_data);

        glUseProgram(shader_program);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        GLuint palette_scale_location =
            glGetUniformLocation(palette_shader_program, "scale");
        glUniform1f(palette_scale_location, 0.1f);

        GLuint palette_offset_location =
            glGetUniformLocation(palette_shader_program, "offset");
        glUniform2f(palette_offset_location, 0, 0);

        GLuint palette_color_location =
            glGetUniformLocation(palette_shader_program, "color");
        glUniform3f(palette_color_location, 1.0f, 0.0f, 0.0f);

        /* glUseProgram(palette_shader_program); */
        /* glBindVertexArray(palette_vao); */
        /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */

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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {

        /* tex_coord_offset[0]&1 */
        printf("cursor_pos: %f %f\n", cursor_pos[0], cursor_pos[1]);

        /* get percentage */
        float px = cursor_pos[0] / WIDTH;
        float py = cursor_pos[1] / HEIGHT;

        printf("px,py: %f %f\n", px, py);

        /* actual dimensions */
        float w = WIDTH / zoom_multiplier;
        float h = HEIGHT / zoom_multiplier;

        printf("w,h: %f %f\n", w, h);

        /* find pixel which is % across screen...? */
        /* find top left pixel value... */
        float x0 = WIDTH * tex_coord_offset[0];
        float y0 = HEIGHT * tex_coord_offset[1];

        printf("x0,y0: %f %f\n", x0, x0);

        /* bottom right value */
        float x1 = x0 + w;
        float y1 = y0 + h;

        printf("x1,y1: %f %f\n", x1, y1);
	/*
	  for example we have zoomed into max and have a 64x64
	  square (512x512 canvas).

	  224,224
	  |--------------|
	  |              |
	  |              |
	  |              |
	  |              |
	  |              |
	  |--------------|288,288

	  any pixels drawn anywhere between 0,7 and 7,7 will need to be
	  placed at 224,224
	  any pixels drawn between 504,504 and 511,511 will need to be
	  placed at 228,228
         */

        int x = (int)(x0 + cursor_pos[0] / zoom_multiplier);
        int y = (int)(y0 + cursor_pos[1] / zoom_multiplier);

        printf("x,y: %d %d\n", x, y);

        if (x > x0 && x < x1 && y > y0 && y < y1) {
            canvas_data[y][x][0] = 255;
            canvas_data[y][x][1] = 255;
            canvas_data[y][x][2] = 255;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        tex_coord_offset[0] -= 1.0f / WIDTH;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        tex_coord_offset[0] += 1.0f / WIDTH;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        tex_coord_offset[1] += 1.0f / HEIGHT;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        tex_coord_offset[1] -= 1.0f / HEIGHT;
    }

    /* printf("%f %f\n", tex_coord_offset[0], tex_coord_offset[1]); */

    pan_mode = 0;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
      pan_mode = 1;
    }
}

void
mouse_callback(GLFWwindow *window, double x, double y)
{
    if (pan_mode > 0) {
      tex_coord_offset[0] += (cursor_pos_last[0] - cursor_pos[0]) * zoom / WIDTH;
      tex_coord_offset[1] += (cursor_pos_last[1] - cursor_pos[1]) * zoom / HEIGHT;
    }

    cursor_pos_last[0] = cursor_pos[0];
    cursor_pos_last[1] = cursor_pos[1];
    cursor_pos[0] = x;
    cursor_pos[1] = y;
}

void
mouse_button_callback(GLFWwindow *window, int a, int b, int c)
{
}

void
scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (yoffset > 0)
        adjust_zoom(1);
    else
        adjust_zoom(0);
}
