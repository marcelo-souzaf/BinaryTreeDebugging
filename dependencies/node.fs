#version 300 es

precision mediump float;

out vec4 color;

uniform vec4 rgba;

void main() {
    color = rgba;
}