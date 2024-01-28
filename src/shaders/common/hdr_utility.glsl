
// Calculates the luminance of a sample using specific weights per color channel
// Note that the sum of the per-channel luminance multipliers must equal 1.0
float Luma(vec3 color)
{
	vec3 lumaMultiplier = vec3(0.21, 0.72, 0.07);
	return dot(color, lumaMultiplier);
}