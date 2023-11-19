#ifndef SHADER_DATA_H
#define SHADER_DATA_H

#include <string>

namespace TANG
{
	namespace SHADER
	{
		enum class DataType
		{
			UNIFORM_BUFFER,
			IMAGE_SAMPLER
		};

		enum class ShaderType
		{
			VERTEX_SHADER,
			PIXEL_SHADER
		};
	}

	struct ShaderData
	{
		std::string name;
		SHADER::DataType dataType;
		SHADER::ShaderType shaderType;
		uint32_t set;
		uint32_t binding;
	};

}

#endif