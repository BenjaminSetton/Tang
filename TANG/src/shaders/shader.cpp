
#include <filesystem>
#include <json/json.hpp>
#include <unordered_map>

#include "shader.h"

#include "../config.h"
#include "../device_cache.h"
#include "../utils/file_utils.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"

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
	{ TANG::ShaderType::BLOOM_PREFILTER			, "bloom_prefilter"			},
	{ TANG::ShaderType::BLOOM_UPSCALING			, "bloom_upscaling"			},
	{ TANG::ShaderType::BLOOM_DOWNSCALING		, "bloom_downscaling"		},
	{ TANG::ShaderType::BLOOM_COMPOSITION		, "bloom_composition"		},
};

static const std::unordered_map<TANG::ShaderStage, std::string> ShaderStageToFileName =
{
	{ TANG::ShaderStage::VERTEX_SHADER			, "vert" },
	{ TANG::ShaderStage::GEOMETRY_SHADER		, "geom" },
	{ TANG::ShaderStage::FRAGMENT_SHADER		, "frag" },
	{ TANG::ShaderStage::COMPUTE_SHADER			, "comp" },
};

namespace TANG
{
	struct ShaderLayoutEntry
	{
		std::string name;
		std::unordered_map<std::string, uint32_t> qualifiers;
		std::vector<std::string> attributes;
	};

	Shader::Shader(const ShaderType& _type, const ShaderStage& _stage) : m_object(VK_NULL_HANDLE)
	{
		Create(_type, _stage);
		m_type = _type;
		m_stage = _stage;
	}

	Shader::~Shader()
	{ 
		Destroy();
	}

	Shader::Shader(Shader&& other) noexcept : m_object(std::move(other.m_object)), m_type(other.m_type), 
		m_stage(other.m_stage), m_layoutEntries(other.m_layoutEntries)
	{ }

	void Shader::Create(const ShaderType& _type, const ShaderStage& _stage)
	{
		namespace fs = std::filesystem;

		const std::string& fileName = ShaderStageToFileName.at(_stage);
		bool readSuccessful = false;

		const std::string fileByteCode = fileName + std::string(".spv");
		const std::string fullShaderByteCodePath = (fs::path(CONFIG::CompiledShaderOutputPath) / fs::path(ShaderTypeToFolderName.at(_type)) / fs::path(fileByteCode.data())).generic_string();
		readSuccessful = ReadShaderByteCode(fullShaderByteCodePath);
		if (!readSuccessful)
		{
			LogError("Failed to create shader (type '%u' and stage '%u') from file '%s'", static_cast<uint32_t>(_type), static_cast<uint32_t>(_stage), fullShaderByteCodePath.data());
			return;
		}

		const std::string fileMetadata = fileName + std::string(".meta");
		const std::string fullShaderMetadataPath = (fs::path(CONFIG::CompiledShaderOutputPath) / fs::path(ShaderTypeToFolderName.at(_type)) / fs::path(fileMetadata.data())).generic_string();
		readSuccessful = true; // ReadShaderMetadata(fullShaderMetadataPath);
		if (!readSuccessful)
		{
			LogError("Failed to read shader metadata (type '%u' and stage '%u') from file '%s'", static_cast<uint32_t>(_type), static_cast<uint32_t>(_stage), fullShaderByteCodePath.data());
			return;
		}

	}

	void Shader::Destroy()
	{
		vkDestroyShaderModule(GetLogicalDevice(), m_object, nullptr);
	}

	bool Shader::ReadShaderByteCode(std::string_view byteCodePath)
	{
		VkDevice logicalDevice = GetLogicalDevice();

		char* shaderCode = nullptr;
		uint32_t bytesRead = ReadFile(byteCodePath, &shaderCode);
		if (bytesRead == 0)
		{
			return false;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = bytesRead;
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode);

		VkResult createCode = vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &m_object);

		delete[] shaderCode;
		return (createCode == VK_SUCCESS);
	}

	bool Shader::ReadShaderMetadata(std::string_view metadataPath)
	{
		char* metadata = nullptr;
		uint32_t bytesRead = ReadFile(metadataPath, &metadata);
		if (bytesRead == 0)
		{
			return false;
		}

		nlohmann::json data = nlohmann::json::parse(metadata, metadata + bytesRead);
		
		// Now that we have the data, loop over all layout objects
		for (auto& layoutObj : data)
		{
			ShaderLayoutEntry entry;
			entry.name = layoutObj["name"].get<std::string>();

			// Loop over all qualifiers
			nlohmann::json qualifiers = layoutObj["qualifiers"];
			TNG_ASSERT(qualifiers.is_object());
			try
			{
				for (auto& qualifierIter : qualifiers.items())
				{
					// Only iterators have access to both key() and value(), so when iterating over dictionaries/maps
					// we must use an iterator over the returned object after calling .items()
					if (qualifierIter.value().is_null())
					{
						// Not sure how to explicitly convert a "null" value to an int (0) without an exception
						// being thrown. With my current knowledge of the library and research, best I can do is this
						entry.qualifiers[qualifierIter.key()] = 0;
					}
					else
					{
						entry.qualifiers[qualifierIter.key()] = qualifierIter.value();
					}
				}
			}
			catch (const std::exception& e)
			{
				// Catch any unintended type conversion errors (apart from casting null to 0, handled explicitly above)
				LogError("[JSON] %s", e.what());
			}

			// Copy over the list of attributes
			nlohmann::json attributes = layoutObj["attributes"];
			TNG_ASSERT(attributes.is_array());
			for (auto& attribute : attributes)
			{
				entry.attributes.push_back(attribute.get<std::string>());
			}

			// Finally, add the entry to the list
			m_layoutEntries.push_back(entry);
		}

		return true;
	} 

	VkShaderModule Shader::GetShaderObject() const
	{
		return m_object;
	}

	ShaderType Shader::GetShaderType() const
	{
		return m_type;
	}

	ShaderStage Shader::GetShaderStage() const
	{
		return m_stage;
	}

	bool Shader::IsValid() const
	{
		return (m_object != VK_NULL_HANDLE);
	}
}


