#version 330 core
out vec4 frag_color;

in vec3 out_color;
in vec2 out_tex_coords;

uniform sampler2D a_texture;
uniform vec2 offset;
uniform float zoom;

void main()
{
    frag_color = texture(a_texture, out_tex_coords * zoom + offset);
}