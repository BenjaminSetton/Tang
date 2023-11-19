#ifndef PBR_DATA_H
#define PBR_DATA_H

#include "../shader_data.h"

namespace TANG
{
	ShaderData data[] =
	{
		// { name,				dataType,                        shaderType,						set, binding }
		{ "DiffuseTexture",		SHADER::DataType::IMAGE_SAMPLER, SHADER::ShaderType::PIXEL_SHADER,		0, 0 },
		{ "NormalTexture",		SHADER::DataType::IMAGE_SAMPLER, SHADER::ShaderType::PIXEL_SHADER,		0, 1 },
		{ "MetallicTexture",	SHADER::DataType::IMAGE_SAMPLER, SHADER::ShaderType::PIXEL_SHADER,		0, 2 },
		{ "RoughnessTexture",	SHADER::DataType::IMAGE_SAMPLER, SHADER::ShaderType::PIXEL_SHADER,		0, 3 },
		{ "LightmapTexture",	SHADER::DataType::IMAGE_SAMPLER, SHADER::ShaderType::PIXEL_SHADER,		0, 4 },

		{ "ProjectionMatrix",	SHADER::DataType::UNIFORM_BUFFER, SHADER::ShaderType::VERTEX_SHADER,	1, 0 },

		{ "CameraData",			SHADER::DataType::UNIFORM_BUFFER, SHADER::ShaderType::PIXEL_SHADER,		2, 0 },
		{ "TransformMatrix",	SHADER::DataType::UNIFORM_BUFFER, SHADER::ShaderType::VERTEX_SHADER,	2, 1 },
		{ "ViewMatrix",			SHADER::DataType::UNIFORM_BUFFER, SHADER::ShaderType::VERTEX_SHADER,	2, 2 },
	};
}

#endif