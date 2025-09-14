#version 450
layout(push_constant) uniform Push {
  mat4 mvp;
  vec4 color;
  vec3 p0;
  float pad0;
  vec3 p1;
} push;
layout(location = 0) out vec4 vColor;
void main() {
  vColor = push.color;
  vec3 pos = (gl_VertexIndex==0) ? push.p0 : push.p1;
  gl_Position = push.mvp * vec4(pos,1.0);
}
