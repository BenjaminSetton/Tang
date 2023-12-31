#ifndef BASE_PIPELINE_H
#define BASE_PIPELINE_H

#include <utility>

#include "../descriptors/set_layout/set_layout_cache.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	enum class PipelineType
	{
		PBR,
		CUBEMAP_PREPROCESSING,
		SKYBOX,
		FULLSCREEN_QUAD
	};

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
		virtual void Create() = 0;
		virtual void Destroy();

		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;

	protected:

		[[nodiscard]] bool CreatePipelineObject(const VkGraphicsPipelineCreateInfo& pipelineCreateInfo);
		[[nodiscard]] bool CreatePipelineLayout(const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo);

		virtual void FlushData() = 0;

		// This should be set to true in every derived class' implementation of SetData(...),
		// and subsequently used inside Create() to return early if the data was not set properly
		bool wasDataSet;

	private:

		VkPipeline pipelineObject;
		VkPipelineLayout pipelineLayout;
	};
}

#endif