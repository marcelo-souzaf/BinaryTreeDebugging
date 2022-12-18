#version 300 es

precision mediump float;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texture_coordinates;
out vec2 coords;

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(position, 0.0, 1.0);
    coords = texture_coordinates;
}