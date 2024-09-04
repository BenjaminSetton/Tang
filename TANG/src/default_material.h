#ifndef DEFAULT_MATERIAL_H
#define DEFAULT_MATERIAL_H

#include <unordered_map>

#include "asset_types.h"

namespace TANG
{
	// Takes in the separate color channels 
	constexpr uint32_t Color_AsRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		uint32_t alpha	= static_cast<uint32_t>(a) << 24;
		uint32_t blue	= static_cast<uint32_t>(b) << 16;
		uint32_t green	= static_cast<uint32_t>(g) << 8;
		uint32_t red	= static_cast<uint32_t>(r) << 0;
		return (alpha | blue | green | red);
	}

	constexpr uint32_t Color_AsFloat(float r, float g, float b, float a)
	{
		// Adjust all parameters
		float red = std::min(std::max(r, 0.0f), 1.0f);
		float green = std::min(std::max(g, 0.0f), 1.0f);
		float blue = std::min(std::max(b, 0.0f), 1.0f);
		float alpha = std::min(std::max(a, 0.0f), 1.0f);
		return Color_AsRGBA(
			static_cast<uint8_t>(255.0f * red),
			static_cast<uint8_t>(255.0f * green),
			static_cast<uint8_t>(255.0f * blue),
			static_cast<uint8_t>(255.0f * alpha)
		);
	}

	// Takes in an 8-bit RGB and 8-bit alpha and converts it to a grayscale color
	constexpr uint32_t Color_GrayscaleAsByte(uint8_t RGB, uint8_t A)
	{
		return Color_AsRGBA(RGB, RGB, RGB, A);
	}

	// Takes in a float between 0 and 1 and converts it to a grayscale color with an alpha of 1
	// NOTE - This function will clamp the parameter between 0 and 1, if not inside the range
	constexpr uint32_t Color_GrayscaleAsFloat(float RGB, float A)
	{
		float RGBAdjusted = std::min(std::max(RGB, 0.0f), 1.0f);
		float AAdjusted = std::min(std::max(A, 0.0f), 1.0f);
		return Color_GrayscaleAsByte(static_cast<uint8_t>(255.0f * RGBAdjusted), static_cast<uint8_t>(255.0f * AAdjusted));
	}

	// Defines the default material's textures, mapped from texture type to an RGBA color
	static const std::unordered_map<Material::TEXTURE_TYPE, uint32_t> DEFAULT_MATERIAL =
	{
		{ Material::TEXTURE_TYPE::DIFFUSE,				Color_AsFloat(1.0f, 0.0f, 0.48f, 1.0f) },	// Light pink
		{ Material::TEXTURE_TYPE::SPECULAR,				Color_GrayscaleAsFloat(0.2f, 1.0f) },		// Low-ish specular
		{ Material::TEXTURE_TYPE::NORMAL,				Color_GrayscaleAsFloat(0.0f, 1.0f) },		// Invalid normal (mesh normals are used instead)
		{ Material::TEXTURE_TYPE::AMBIENT_OCCLUSION,	Color_GrayscaleAsFloat(1.0f, 1.0f) },		// No occlusions
		{ Material::TEXTURE_TYPE::METALLIC,				Color_GrayscaleAsFloat(0.15f, 1.0f) },		// Low metallic
		{ Material::TEXTURE_TYPE::ROUGHNESS,			Color_GrayscaleAsFloat(0.33f, 1.0f) },		// High roughness
		{ Material::TEXTURE_TYPE::LIGHTMAP,				Color_GrayscaleAsFloat(1.0f, 1.0f) },		// ???? I don't even know what this is, to be honest
		{ Material::TEXTURE_TYPE::_COUNT,				Color_GrayscaleAsFloat(0.0f, 0.0f) },		// Invalid
	};
}


#endif