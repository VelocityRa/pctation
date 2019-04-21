#version 330 core

uniform sampler2D u_vram;

in vec2 v_texcoord;

out vec4 o_color;

const float VRAM_WIDTH = 1024;
const float VRAM_HEIGHT = 512;

void main() {
    vec2 vram_texcoord = v_texcoord * vec2(VRAM_WIDTH, VRAM_HEIGHT);
    vec3 vram_value = texelFetch(u_vram, ivec2(vram_texcoord), 0).rgb;

    o_color = vec4(vram_value, 1.0);
}
