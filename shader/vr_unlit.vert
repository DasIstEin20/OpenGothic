#version 450
layout(location = 0) in vec3 inPos;
layout(push_constant) uniform Push {
  mat4 mvp;
  vec4 color;
} push;
layout(location = 0) out vec4 vColor;
void main() {
  vColor = push.color;
  gl_Position = push.mvp * vec4(inPos,1.0);
}
