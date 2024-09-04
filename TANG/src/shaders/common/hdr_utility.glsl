
vec3 PowVec(vec3 vec, float p)
{
	return vec3(pow(vec.x, p), pow(vec.y, p), pow(vec.z, p));
}

const float INV_GAMMA = 1.0 / 2.2;
vec3 RGBToSRGB(vec3 rgb)
{
	return PowVec(rgb, INV_GAMMA);
}

// Calculates the luminosity of a sample using specific weights per color channel
// Note that the sum of the per-channel luminosity multipliers must equal 1.0
float Luma(vec3 color)
{
	vec3 lumaMultiplier = vec3(0.21, 0.72, 0.07);
	return dot(color, lumaMultiplier);
}