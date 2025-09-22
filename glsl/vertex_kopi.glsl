#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

uniform vec2 offset;
uniform float angle;
uniform float aspect;

void main() {
  // Reconcile non-square aspect ratio
  vec2 recd = vec2(aPos.x / aspect, aPos.y);
  float c = cos(angle);
  float s = sin(angle);
  vec2 rotated = vec2(
    recd.x * c - recd.y * s,
    recd.x * s + recd.y * c
  );
  // Scale back to NDC space
  rotated.x *= aspect;
  gl_Position = vec4(rotated + offset, 0.0, 1.0);
  TexCoord = aTexCoord;
}