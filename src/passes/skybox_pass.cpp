
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/base_render_pass.h"
#include "skybox_pass.h"

static const glm::mat4 cubemapProjMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

static const glm::mat4 cubemapViewMatrices[] =
{
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // RIGHT
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // LEFT
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // DOWN
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // UP
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // RIGHT
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // LEFT
};

namespace TANG
{
	SkyboxPass::SkyboxPass()
	{
	}

	SkyboxPass::~SkyboxPass()
	{
	}

	SkyboxPass::SkyboxPass(SkyboxPass&& other) noexcept
	{

	}

	void SkyboxPass::Create(const DescriptorPool& descriptorPool)
	{
		CreateFramebuffers();
		CreatePipelines();
		CreateRenderPasses();
		CreateSetLayoutCaches();
		CreateDescriptorSets(descriptorPool);
		CreateUniformBuffers();
		CreateSyncObjects();

		InitializeShaderParameters();
	}

	void SkyboxPass::Destroy()
	{
		skyboxSetLayoutCache.DestroyLayouts();
	}

	void SkyboxPass::Draw(uint32_t currentFrame, const DrawData& data)
	{
		if (!IsDrawDataValid(data))
		{
			return;
		}

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.pNext = nullptr;
		inheritanceInfo.renderPass = data.renderPass->GetRenderPass();
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = data.framebuffer->GetFramebuffer();

		data.cmdBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, &inheritanceInfo);

		data.cmdBuffer->CMD_SetScissor({ 0, 0 }, {data.framebufferWidth, data.framebufferHeight});
		data.cmdBuffer->CMD_SetViewport(static_cast<float>(data.framebufferWidth), static_cast<float>(data.framebufferHeight));
		data.cmdBuffer->CMD_BindGraphicsPipeline(skyboxPipeline.GetPipeline());
		data.cmdBuffer->CMD_BindMesh(data.asset);
		data.cmdBuffer->CMD_BindDescriptorSets(skyboxPipeline.GetPipelineLayout(), 3, reinterpret_cast<VkDescriptorSet*>(skyboxDescriptorSets[currentFrame].data()));
		data.cmdBuffer->CMD_DrawIndexed(static_cast<uint32_t>(data.asset->indexCount));

		data.cmdBuffer->EndRecording();
	}

	void SkyboxPass::CreatePipelines()
	{
		skyboxPipeline.SetData(&hdrRenderPass, &skyboxSetLayoutCache, swapChainExtent);
		skyboxPipeline.Create();
	}

	void SkyboxPass::CreateSetLayoutCaches()
	{
		// Skybox
		{
			SetLayoutSummary persistentLayout;
			persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Skybox texture
			skyboxSetLayoutCache.CreateSetLayout(persistentLayout, 0);

			SetLayoutSummary volatileLayout;
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
			skyboxSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}
	}

	void SkyboxPass::CreateUniformBuffers()
	{
		// Skybox
		const LayoutCache& cache = skyboxSetLayoutCache.GetLayoutCache();
		if (skyboxSetLayoutCache.GetLayoutCount() != 3)
		{
			TNG_ASSERT_MSG(false, "Failed to create skybox descriptor set!");
			return;
		}

		for (uint32_t i = 0; i < GetFDDSize(); i++)
		{
			auto frameData = GetFDDAtIndex(i);
			uint32_t j = 0;
			for (auto& iter : cache)
			{
				frameData->skyboxDescriptorSets[j].Create(descriptorPool, iter.second);
				j++;
			}
		}
	}