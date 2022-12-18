#version 300 es

precision mediump float;

in vec2 coords;
out vec4 color;

uniform sampler2D text;  // mono-colored bitmap image of the glyph

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, coords).r);
    color = vec4(0.0, 0.0, 0.0, 1.0) * sampled;
}