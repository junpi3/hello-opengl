#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

uniform float zoom;
uniform vec2 pan;

void main() {
  gl_Position = vec4(aPos, 0.0, 1.0);
  TexCoord = (aTexCoord - 0.5) / zoom + 0.5 + pan;
}