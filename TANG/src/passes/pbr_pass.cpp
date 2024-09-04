
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/base_render_pass.h"
#include "pbr_pass.h"

namespace TANG
{
	PBRPass::PBRPass()
	{ }

	PBRPass::~PBRPass()
	{ }

	PBRPass::PBRPass(PBRPass&& other) noexcept
	{
		UNUSED(other);
		TNG_TODO();
	}

	void PBRPass::SetData(const DescriptorPool* descriptorPool, const HDRRenderPass* hdrRenderPass, VkExtent2D swapChainExtent)
	{
		borrowedData.descriptorPool = descriptorPool;
		borrowedData.hdrRenderPass = hdrRenderPass;
		borrowedData.swapChainExtent = swapChainExtent;
	}

	void UpdatePBRTextureDescriptors(const TextureResource* skyboxCubemap[8])
	{
		/*for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			WriteDescriptorSets writeSetPersistent(0, 1);
			writeSetPersistent.AddImage(skyboxDescriptorSets[i][0].GetDescriptorSet(), 0, skyboxCubemap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
			skyboxDescriptorSets[i][0].Update(writeSetPersistent);
		}*/
	}

	//void PBRPass::UpdateCameraMatricesShaderParameters(uint32_t frameIndex, const UniformBuffer* view, const UniformBuffer* proj)
	//{
	//	WriteDescriptorSets writeSetVolatile(2, 0);
	//	writeSetVolatile.AddUniformBuffer(skyboxDescriptorSets[frameIndex][1].GetDescriptorSet(), 0, view);
	//	writeSetVolatile.AddUniformBuffer(skyboxDescriptorSets[frameIndex][1].GetDescriptorSet(), 1, proj);
	//	skyboxDescriptorSets[frameIndex][1].Update(writeSetVolatile);
	//}

	void PBRPass::Create()
	{
		if (wasCreated)
		{
			LogWarning("Attempting to create pbr pass more than once!");
			return;
		}

		ResetBaseMembers();

		CreateSetLayoutCaches();
		CreateUniformBuffers();
		CreateDescriptorSets();
		CreateSyncObjects();
		CreateRenderPasses();
		CreatePipelines();
		CreateFramebuffers();

		ResetBorrowedData();
		wasCreated = true;
	}

	void PBRPass::Destroy()
	{
		pbrSetLayoutCache.DestroyLayouts();
		pbrPipeline.Destroy();
	}

	void PBRPass::Draw(uint32_t currentFrame, const DrawData& data)
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

		data.cmdBuffer->CMD_SetScissor({ 0, 0 }, { data.framebufferWidth, data.framebufferHeight });
		data.cmdBuffer->CMD_SetViewport(static_cast<float>(data.framebufferWidth), static_cast<float>(data.framebufferHeight));
		data.cmdBuffer->CMD_BindPipeline(&pbrPipeline);
		data.cmdBuffer->CMD_BindMesh(data.asset);
		data.cmdBuffer->CMD_BindDescriptorSets(&pbrPipeline, static_cast<uint32_t>(pbrDescriptorSets.size()), reinterpret_cast<VkDescriptorSet*>(pbrDescriptorSets[currentFrame].data()));
		data.cmdBuffer->CMD_DrawIndexed(data.asset->indexCount);

		data.cmdBuffer->EndRecording();
	}

	void PBRPass::CreatePipelines()
	{
		pbrPipeline.SetData(borrowedData.hdrRenderPass, &pbrSetLayoutCache, borrowedData.swapChainExtent);
		pbrPipeline.Create();
	}

	void PBRPass::CreateSetLayoutCaches()
	{
		SetLayoutSummary persistentLayout(0);
		persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Skybox texture
		pbrSetLayoutCache.CreateSetLayout(persistentLayout, 0);

		SetLayoutSummary volatileLayout(1);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
		pbrSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	void PBRPass::CreateDescriptorSets()
	{
		uint32_t pbrSetLayoutCount = pbrSetLayoutCache.GetLayoutCount();
		if (pbrSetLayoutCount != 2)
		{
			LogError("Failed to create pbr descriptor set, invalid set layout count! Expected (%u) vs. actual (%u)", 2, pbrSetLayoutCount);
			return;
		}

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			for (uint32_t j = 0; j < pbrSetLayoutCount; j++)
			{
				std::optional<DescriptorSetLayout> setLayoutOpt = pbrSetLayoutCache.GetSetLayout(j);
				if (!setLayoutOpt.has_value())
				{
					LogWarning("Failed to create pbr descriptor set! Set layout at %u was null", j);
					continue;
				}

				pbrDescriptorSets[i][j].Create(*(borrowedData.descriptorPool), setLayoutOpt.value());
			}
		}
	}

	void PBRPass::ResetBorrowedData()
	{
		memset(&borrowedData, 0, sizeof(borrowedData));
	}
}