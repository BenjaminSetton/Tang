
#include <filesystem>
#include <unordered_map>

#include "device_cache.h"
#include "shader.h"
#include "utils/file_utils.h"
#include "utils/sanity_check.h"

static const std::string CompiledShaderOutputPath = "./shaders";

static const std::unordered_map<TANG::ShaderType, std::string> ShaderTypeToFolderName =
{
	{ TANG::ShaderType::PBR, "pbr" },
	{ TANG::ShaderType::SKYBOX, "skybox" },
};

namespace TANG
{

	Shader::Shader() : shaderObject(VK_NULL_HANDLE)
	{ }

	Shader::~Shader()
	{ }

	Shader::Shader(Shader&& other) noexcept : shaderObject(std::move(other.shaderObject))
	{ }

	void Shader::Create(const std::string_view& fileName, const ShaderType& type)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		namespace fs = std::filesystem;
		const std::string defaultShaderCompiledPath = (fs::path(CompiledShaderOutputPath) / fs::path(ShaderTypeToFolderName.at(type)) / fs::path(fileName.data())).generic_string();

		auto shaderCode = ReadFile(defaultShaderCompiledPath);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = static_cast<uint32_t>(shaderCode.size());
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderObject) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create shader!");
		}
	}

	void Shader::Destroy()
	{
		vkDestroyShaderModule(GetLogicalDevice(), shaderObject, nullptr);
	}

	VkShaderModule Shader::GetShaderObject() const
	{
		return shaderObject;
	}
}


