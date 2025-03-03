#version 330 core 
in vec3 fragNor;
in float fragDensity;
in float fragMaxDensity;
out vec4 color;

void main()
{
	// vec3 normal = normalize(fragNor);
	// Map normal in the range [-1, 1] to color in range [0, 1];
	// vec3 Ncolor = 0.5*normal + 0.5;
	// color = vec4(Ncolor, 1.0);

	color = vec4(fragDensity / fragMaxDensity, 0, (fragMaxDensity - fragDensity) / fragMaxDensity, 1.0);
}
