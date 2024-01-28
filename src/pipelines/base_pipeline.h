#ifndef BASE_PIPELINE_H
#define BASE_PIPELINE_H

#include <utility>

#include "../vertex_types.h"
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

	// Forward declarations
	class Shader;

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

		// Populates a generic VkPipelineInputAssemblyStateCreateInfo struct with a triangle strip
		VkPipelineInputAssemblyStateCreateInfo PopulateInputAssemblyCreateInfo(VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 primitiveRestartEnable = VK_FALSE) const;

		VkPipelineDynamicStateCreateInfo PopulateDynamicStateCreateInfo(const VkDynamicState* dynamicStates = nullptr, uint32_t dynamicStateCount = 0) const;

		VkPipelineViewportStateCreateInfo PopulateViewportStateCreateInfo(const VkViewport* viewports, uint32_t viewportCount, const VkRect2D* scissors, uint32_t scissorCount) const;

		VkPipelineRasterizationStateCreateInfo PopulateRasterizerStateCreateInfo(VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT, VkFrontFace windingOrder = VK_FRONT_FACE_COUNTER_CLOCKWISE);

		VkPipelineMultisampleStateCreateInfo PopulateMultisamplingStateCreateInfo(VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

		VkPipelineColorBlendAttachmentState PopulateColorBlendAttachment(); // No need for parameters yet...

		VkPipelineColorBlendStateCreateInfo PopulateColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState* attachments, uint32_t blendAttachmentCount);

		VkPipelineDepthStencilStateCreateInfo PopulateDepthStencilStateCreateInfo(VkBool32 depthTestEnable = VK_FALSE, VkBool32 depthWriteEnable = VK_FALSE, VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL, VkBool32 depthBoundsTestEnable = VK_FALSE, VkBool32 stencilTestEnable = VK_FALSE);

		VkPipelineLayoutCreateInfo PopulatePipelineLayoutCreateInfo(const VkDescriptorSetLayout* setLayouts, uint32_t setLayoutCount, const VkPushConstantRange* pushConstantRanges, uint32_t pushConstantCount);

		// Populates a VkPipelineShaderStageCreateInfo with shader information depending on the shader parameter
		VkPipelineShaderStageCreateInfo PopulateShaderCreateInfo(const Shader& shader) const;

		VkViewport PopulateViewportInfo(uint32_t width, uint32_t height) const;

		VkRect2D PopulateScissorInfo(VkExtent2D viewportSize) const;

		template<typename T>
		VkPipelineVertexInputStateCreateInfo PopulateVertexInputCreateInfo()
		{
			if constexpr (std::is_base_of_v<VertexType, T>)
			{
				auto& bindingDescription = T::GetBindingDescription();
				auto& attributeDescriptions = T::GetAttributeDescriptions();

				VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputInfo.vertexBindingDescriptionCount = 1;
				vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
				vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

				return vertexInputInfo;
			}
			
			// If this function did not compile, please use a derived VertexType!
		}

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