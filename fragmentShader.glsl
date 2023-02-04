#version 330 core

uniform vec3 color;
in vec3 oN;
in vec3 oFragPos;

void main()
{
	float as = 0.2;

	vec3 lc = vec3(1.0, 1.0, 1.0);
	vec3 lightLocation = vec3(0.0);

	vec3 ldir = normalize(lightLocation - oFragPos);
	float diff = max(dot(oN, ldir), 0.0);

	vec3 amb = lc * as;

	vec2 uv = gl_FragCoord.xy / vec2(600, 600);
	gl_FragColor = vec4(((amb + ((1.0 - diff) * lc)) * color) , 1.0); 
}	