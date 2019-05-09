#version 330 core

uniform sampler2D u_vram;
uniform vec2 u_tex_size;

in vec2 v_texcoord;

out vec4 o_color;

void main() {
    vec2 vram_texcoord = v_texcoord * vec2(u_tex_size.x, u_tex_size.y);
    vec3 vram_value = texelFetch(u_vram, ivec2(vram_texcoord), 0).rgb;

    o_color = vec4(vram_value, 1.0);
}
