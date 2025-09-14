#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant, std140) uniform Push {
  float inner;
} push;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
  vec2 c = uv*2.0 - 1.0;
  float r = length(c);
  float m = smoothstep(push.inner, 1.0, r);
  outColor = vec4(1.0 - m);
}
