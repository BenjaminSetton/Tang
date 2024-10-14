
#include "vulkan/vulkan.h"

#include <string_view>
#include <vector>

namespace TANG
{
	// Forward declaration
	struct ShaderLayoutEntry;

	enum class ShaderType
	{
		PBR,
		CUBEMAP_PREPROCESSING,
		SKYBOX,
		LDR,
		FULLSCREEN_QUAD,
		IRRADIANCE_SAMPLING,
		PREFILTER_MAP,
		BRDF_CONVOLUTION,
		BLOOM_PREFILTER,
		BLOOM_UPSCALING,
		BLOOM_DOWNSCALING,
		BLOOM_COMPOSITION,
	};

	enum class ShaderStage
	{
		VERTEX_SHADER,
		GEOMETRY_SHADER,
		FRAGMENT_SHADER,
		COMPUTE_SHADER
	};

	// Shaders instances are short-lived objects. They're exclusively used to create pipelines, and deleted immediately
	// after. Currently there is no reason to keep shader objects around, so we'll be using RAII. If this changes, switch
	// back to explicit Create()/Destroy() calls
	class Shader
	{
	public:

		// TODO - Replace with a single std::string_view parameter
		explicit Shader(const ShaderType& type, const ShaderStage& stage);
		Shader() = delete;
		~Shader();
		Shader(Shader&& other) noexcept;

		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;

		VkShaderModule GetShaderObject() const;
		ShaderType GetShaderType() const;
		ShaderStage GetShaderStage() const;

		bool IsValid() const;

	private:

		void Create(const ShaderType& type, const ShaderStage& stage);
		void Destroy();

		bool ReadShaderByteCode(std::string_view byteCodePath);
		bool ReadShaderMetadata(std::string_view metadataPath);

		VkShaderModule m_object;
		ShaderType m_type;
		ShaderStage m_stage;
		std::vector<ShaderLayoutEntry> m_layoutEntries;
	};
}