
#include "vulkan/vulkan.h"

#include <string_view>

namespace TANG
{
	enum class ShaderType
	{
		PBR,
		SKYBOX
	};

	class Shader
	{
	public:

		Shader();
		~Shader();
		Shader(Shader&& other) noexcept;

		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;

		void Create(VkDevice logicalDevice, const std::string_view& fileName, const ShaderType& type);
		void Destroy(VkDevice logicalDevice);

		VkShaderModule GetShaderObject() const;

	private:

		VkShaderModule shaderObject;
	};
}