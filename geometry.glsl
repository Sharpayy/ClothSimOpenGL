#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 fragPos[];
out vec3 oN;
out vec3 oFragPos;

void main()
{
	vec3 A = fragPos[1] - fragPos[0];
	vec3 B = fragPos[2] - fragPos[0];
	oN = normalize(vec3(
	A.y * B.z - A.z * B.y,
	A.z * B.x - A.x * B.z,
	A.x * B.y - A.y * B.x
	));
	oN *= sign(oN);

	gl_Position = gl_in[0].gl_Position;
	oFragPos = fragPos[0];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position;
	oFragPos = fragPos[1];
	EmitVertex();

	//oN = -normalize(cross(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz));
	gl_Position = gl_in[2].gl_Position;
	oFragPos = fragPos[2];
	EmitVertex();

	EndPrimitive();
}