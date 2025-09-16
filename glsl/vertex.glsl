#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform vec2 offset;
uniform float angle;
void main() {
  float c = cos(angle);
  float s = sin(angle);
  vec2 rotated = vec2(
    aPos.x * c - aPos.y * s,
    aPos.x * s + aPos.y * c
  );
  gl_Position = vec4(rotated + offset, 0.0, 1.0);
  TexCoord = aTexCoord;
}