#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 model;
uniform mat4 camera;
uniform mat4 proj;

out vec3 fragPos;

void main()
{
	vec4 tp = camera * model * vec4(pos, 1.0);
	fragPos = (model * vec4(pos, 1.0)).xyz;
	gl_Position = proj * tp;
}