#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform float density;
uniform float maxDensity;
out vec3 fragNor;
out float fragDensity;
out float fragMaxDensity;


void main()
{
	gl_Position = P * V * M * vertPos;
	fragNor = (M * vec4(vertNor, 0.0)).xyz;
	fragDensity = density;
	fragMaxDensity = maxDensity;
}
