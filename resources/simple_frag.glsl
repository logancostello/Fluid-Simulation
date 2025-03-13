#version 330 core 
in vec3 fragNor;
in float fragDensityDifference;
out vec4 color;

void main()
{
	// vec3 normal = normalize(fragNor);
	// Map normal in the range [-1, 1] to color in range [0, 1];
	// vec3 Ncolor = 0.5*normal + 0.5;
	// color = vec4(Ncolor, 1.0);

	// // Cell Debugging
	// color = vec4(fragDensityDifference, fragDensityDifference, fragDensityDifference, 1);

	// Density Debugging
	if (fragDensityDifference == 0) {
		color = vec4(1, 1, 1, 1.0);
	} else if (fragDensityDifference > 0) {
		float x = 1 / (1 + fragDensityDifference);
		color = vec4(1, x, x, 1);
	} else {
		float x = 1 / (1 + -1 * fragDensityDifference);
		color = vec4(x, x, 1, 1);
	}

	// color = vec4(1, 1, 1, 1);
}
