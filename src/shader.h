
#include "vulkan/vulkan.h"

#include <string_view>

namespace TANG
{
	enum class ShaderType
	{
		PBR,
		CUBEMAP_PREPROCESSING,
		SKYBOX,
		LDR,
		FULLSCREEN_QUAD,
		IRRADIANCE_SAMPLING,
		PREFILTER_MAP,
		BRDF_CONVOLUTION
	};

	enum class ShaderStage
	{
		VERTEX_SHADER,
		GEOMETRY_SHADER,
		FRAGMENT_SHADER
	};

	// Shaders instances are short-lived objects. They're exclusively used to create pipelines, and deleted immediately
	// after. Currently there is no reason to keep shader objects around, so we'll be using RAII. If this changes, switch
	// back to explicit Create()/Destroy() calls
	class Shader
	{
	public:

		explicit Shader(const ShaderType& type, const ShaderStage& stage);
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

		VkShaderModule object;
		ShaderType type;
		ShaderStage stage;
	};
}