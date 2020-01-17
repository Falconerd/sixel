#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

int
read_file_into_buffer(char **buffer, const char *path)
{
    int fsize;
    char *str; 
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    str = (char *)malloc(fsize + 1);
    fread(str, 1, fsize, file);
    fclose(file);
    str[fsize] = 0;

    *buffer = str;
    return 0;
}

GLuint
create_shader_program(
    const char *vert_path,
    const char *frag_path,
    const char *geom_path)
{
    GLuint shader_program;
    GLuint vert_shader;
    GLuint frag_shader;
    GLuint geom_shader;
    char *vert_shader_source;
    char *frag_shader_source;
    char *geom_shader_source;
    int success;
    char info_log[512];

    read_file_into_buffer(&vert_shader_source, vert_path);
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, (const char * const*)&vert_shader_source, NULL);
    glCompileShader(vert_shader);

    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
        printf("Error compiling vertex shader\n%s\n", info_log);
        return -1;
    }

    read_file_into_buffer(&frag_shader_source, frag_path);
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, (const char*const*)&frag_shader_source, NULL);
    glCompileShader(frag_shader);

    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        printf(
            "Error compiling fragment shader\n%s\n",
            info_log);
        return -1;
    }

    if (geom_path != NULL) {
        read_file_into_buffer(&geom_shader_source, geom_path);
        geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geom_shader, 1, (const char*const*)&geom_shader_source, NULL);
        glCompileShader(geom_shader);

        glGetShaderiv(geom_shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(
                geom_shader,
                512,
                NULL,
                info_log);
            printf(
                "Error compiling geometry shader\n%s\n",
                info_log);
        }
    }

    shader_program = glCreateProgram();

    glAttachShader(shader_program, vert_shader);
    glAttachShader(shader_program, frag_shader);
    if (geom_path != NULL)
        glAttachShader(shader_program, geom_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(
            shader_program,
            512,
            NULL,
            info_log);
        printf("Error linking shader \n%s\n", info_log);
        return -1;
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    if (geom_path != NULL)
        glDeleteShader(geom_shader);

    return shader_program;
}


#endif
