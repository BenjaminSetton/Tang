
#include "../descriptors/set_layout/set_layout_cache.h"
#include "vulkan/vulkan.h"

#include <utility>

namespace TANG
{
	class BasePipeline
	{
	public:

		BasePipeline();
		~BasePipeline();
		BasePipeline(BasePipeline&& other) noexcept;

		BasePipeline(const BasePipeline& other) = delete;
		BasePipeline& operator=(const BasePipeline& other) = delete;

		// Pure virtual creation/destruction methods. Every derived class must specify all the components
		// necessary to create a Vulkan pipeline
		virtual void Create(VkRenderPass renderPass, const SetLayoutCache& setLayoutCache) = 0;
		virtual void Destroy();

		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;

	protected:

		[[nodiscard]] bool CreatePipelineObject(const VkGraphicsPipelineCreateInfo& pipelineCreateInfo);
		[[nodiscard]] bool CreatePipelineLayout(const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo);

	private:

		VkPipeline pipelineObject;
		VkPipelineLayout pipelineLayout;
	};
}