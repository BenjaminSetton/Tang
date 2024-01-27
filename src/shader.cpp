
#include <filesystem>
#include <unordered_map>

#include "config.h"
#include "device_cache.h"
#include "shader.h"
#include "utils/file_utils.h"
#include "utils/sanity_check.h"
#include "utils/logger.h"

static const std::unordered_map<TANG::ShaderType, std::string> ShaderTypeToFolderName =
{
	{ TANG::ShaderType::PBR						, "pbr"						},
	{ TANG::ShaderType::CUBEMAP_PREPROCESSING	, "cubemap_preprocessing"	},
	{ TANG::ShaderType::SKYBOX					, "skybox"					},
	{ TANG::ShaderType::LDR						, "ldr_conversion"			},
	{ TANG::ShaderType::FULLSCREEN_QUAD			, "fullscreen_quad"			},
	{ TANG::ShaderType::IRRADIANCE_SAMPLING		, "irradiance_sampling"		},
	{ TANG::ShaderType::PREFILTER_MAP			, "prefilter_skybox"		},
	{ TANG::ShaderType::BRDF_CONVOLUTION		, "brdf_convolution"		},
};

static const std::unordered_map<TANG::ShaderStage, std::string> ShaderStageToFileName =
{
	{ TANG::ShaderStage::VERTEX_SHADER			, "vert.spv" },
	{ TANG::ShaderStage::GEOMETRY_SHADER		, "geom.spv" },
	{ TANG::ShaderStage::FRAGMENT_SHADER		, "frag.spv" },
};

namespace TANG
{

	Shader::Shader(const ShaderType& _type, const ShaderStage& _stage) : object(VK_NULL_HANDLE)
	{
		Create(_type, _stage);
		type = _type;
		stage = _stage;

	}

	Shader::~Shader()
	{ 
		Destroy();
	}

	Shader::Shader(Shader&& other) noexcept : object(std::move(other.object))
	{ }

	void Shader::Create(const ShaderType& _type, const ShaderStage& _stage)
	{
		VkDevice logicalDevice = GetLogicalDevice();
		const std::string& fileName = ShaderStageToFileName.at(_stage);

		namespace fs = std::filesystem;
		const std::string defaultShaderCompiledPath = (fs::path(CONFIG::CompiledShaderOutputPath) / fs::path(ShaderTypeToFolderName.at(_type)) / fs::path(fileName.data())).generic_string();

		char* shaderCode = nullptr;
		uint32_t bytesRead = ReadFile(defaultShaderCompiledPath, &shaderCode);
		if (bytesRead == 0)
		{
			LogError("Failed to read shader code for shader of type '%u' and stage '%u'", static_cast<uint32_t>(type), static_cast<uint32_t>(_stage));
			return;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = bytesRead;
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode);

		if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &object) != VK_SUCCESS)
		{
			LogError("Failed to create shader of type '%u' for stage '%u'", static_cast<uint32_t>(_type), static_cast<uint32_t>(_stage));
			return;
		}
	}

	void Shader::Destroy()
	{
		vkDestroyShaderModule(GetLogicalDevice(), object, nullptr);
	}

	VkShaderModule Shader::GetShaderObject() const
	{
		return object;
	}

	ShaderType Shader::GetShaderType() const
	{
		return type;
	}

	ShaderStage Shader::GetShaderStage() const
	{
		return stage;
	}

	bool Shader::IsValid() const
	{
		return (object != VK_NULL_HANDLE);
	}
}


