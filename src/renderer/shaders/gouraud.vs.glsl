#version 330 core

layout(location = 0) in ivec2 a_position;
layout(location = 1) in uvec3 a_color;

out vec3 v_color;

void main() {
    // Convert VRAM coordinates (0;1023, 0;511) into
    // OpenGL coordinates (-1;1, -1;1)
    float xpos = (float(a_position.x) / 512) - 1.0;

    // VRAM puts 0 at the top, OpenGL at the bottom,
    // we must mirror vertically
    float ypos = 1.0 - (float(a_position.y) / 256);

    // Write vertex position
    gl_Position = vec4(xpos, ypos, 0.0, 1.0);

    // Convert the components from [0;255] to [0;1]
    v_color = vec3(float(a_color.r) / 255,
                   float(a_color.g) / 255,
                   float(a_color.b) / 255);
}
