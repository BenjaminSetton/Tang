
#include "vulkan/vulkan.h"

#include <string_view>

namespace TANG
{
	enum class ShaderType
	{
		PBR,
		CUBEMAP_PREPROCESSING
	};

	// Shaders instances are short-lived objects. They're exclusively used to create pipelines, and deleted immediately
	// after. Currently there is no reason to keep shader objects around, so we'll be using RAII. If this changes, switch
	// back to explicit Create()/Destroy() calls
	class Shader
	{
	public:

		explicit Shader(const std::string_view& fileName, const ShaderType& type);
		~Shader();
		Shader(Shader&& other) noexcept;

		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;

		VkShaderModule GetShaderObject() const;

	private:

		void Create(const std::string_view& fileName, const ShaderType& type);
		void Destroy();

		VkShaderModule shaderObject;
	};
}