
// DISABLE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)

#include <glm/gtx/euler_angles.hpp>

#pragma warning(pop)

#include "../asset_types.h"
#include "../ubo_structs.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/base_render_pass.h"
#include "../render_passes/hdr_render_pass.h"
#include "pbr_pass.h"

#include "../tang.h"

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

	void PBRPass::UpdateTransformUniformBuffer(uint32_t frameIndex, Transform& transform)
	{
		// Construct and update the transform UBO
		TransformUBO ubo{};

		glm::mat4 finalTransform = glm::identity<glm::mat4>();
		glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.position);
		glm::mat4 rotation = glm::eulerAngleXYZ(transform.rotation.x, transform.rotation.y, transform.rotation.z);
		glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.scale);

		ubo.transform = translation * rotation * scale;
		transformUBO[frameIndex].UpdateData(&ubo, sizeof(TransformUBO));
	}

	void PBRPass::UpdateViewUniformBuffer(uint32_t frameIndex, const glm::mat4& viewMatrix)
	{
		ViewUBO ubo{};
		ubo.view = viewMatrix;
		viewUBO[frameIndex].UpdateData(&ubo, sizeof(ViewUBO));
	}

	void PBRPass::UpdateProjUniformBuffer(uint32_t frameIndex, const glm::mat4& projMatrix)
	{
		ProjUBO ubo{};
		ubo.proj = projMatrix;
		projUBO[frameIndex].UpdateData(&ubo, sizeof(ProjUBO));
	}

	void PBRPass::UpdateCameraUniformBuffer(uint32_t frameIndex, const glm::vec3& position)
	{
		CameraDataUBO ubo{};
		ubo.position = glm::vec4(position, 1.0f);
		ubo.exposure = 1.0f;
		cameraDataUBO[frameIndex].UpdateData(&ubo, sizeof(CameraDataUBO));
	}

	void PBRPass::UpdateDescriptorSets(uint32_t frameIndex, std::array<const TextureResource*, 8>& textures)
	{
		// SET 0 - PBR textures
		{
			DescriptorSet& descSet = pbrDescriptorSets[frameIndex][0];
			WriteDescriptorSets writeDescSets(0, 8);
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 0, textures[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Diffuse
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 1, textures[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Normal
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 2, textures[2], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Metallic
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 3, textures[3], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Roughness
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 4, textures[4], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Lightmap
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 5, textures[5], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Irradiance map
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 6, textures[6], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // Prefilter map
			writeDescSets.AddImage(descSet.GetDescriptorSet(), 7, textures[7], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0); // BRDF convolution
			descSet.Update(writeDescSets);
		}

		// SET 1 - Projection
		{
			DescriptorSet& descSet = pbrDescriptorSets[frameIndex][1];

			// Update ProjUBO descriptor set
			WriteDescriptorSets writeDescSets(1, 0);
			writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &projUBO[frameIndex]);
			descSet.Update(writeDescSets);
		}


		// SET 2 - transform + cameraData + view
		{
			DescriptorSet& descSet = pbrDescriptorSets[frameIndex][2];
			WriteDescriptorSets writeDescSets(3, 0);
			writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &transformUBO[frameIndex]);
			writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 1, &cameraDataUBO[frameIndex]);
			writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 2, &viewUBO[frameIndex]);
			descSet.Update(writeDescSets);
		}
	}

	void PBRPass::Create(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight)
	{
		if (wasCreated)
		{
			LogWarning("Attempting to create pbr pass more than once!");
			return;
		}

		CreateSetLayoutCaches();
		CreateUniformBuffers();
		CreateDescriptorSets();
		CreatePipelines(hdrRenderPass, swapChainWidth, swapChainHeight);

		wasCreated = true;
	}

	void PBRPass::Destroy()
	{
		pbrSetLayoutCache.DestroyLayouts();
		pbrPipeline.Destroy();
		pbrSetLayoutCache.DestroyLayouts();

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			transformUBO[i].Destroy();
			viewUBO[i].Destroy();
			projUBO[i].Destroy();
			cameraDataUBO[i].Destroy();
		}
	}

	void PBRPass::Draw(uint32_t frameIndex, const DrawData& data)
	{
		if (!data.IsValid())
		{
			return;
		}

		auto& descSets = pbrDescriptorSets[frameIndex];

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.pNext = nullptr;
		inheritanceInfo.renderPass = data.renderPass->GetRenderPass();
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = data.framebuffer->GetFramebuffer();

		data.cmdBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, &inheritanceInfo);

		data.cmdBuffer->CMD_BindMesh(data.asset);
		data.cmdBuffer->CMD_BindDescriptorSets(&pbrPipeline, static_cast<uint32_t>(descSets.size()), descSets.data());
		data.cmdBuffer->CMD_BindPipeline(&pbrPipeline);
		data.cmdBuffer->CMD_SetScissor({ 0, 0 }, { data.framebufferWidth, data.framebufferHeight });
		data.cmdBuffer->CMD_SetViewport(static_cast<float>(data.framebufferWidth), static_cast<float>(data.framebufferHeight));
		data.cmdBuffer->CMD_DrawIndexed(data.asset->indexCount);

		data.cmdBuffer->EndRecording();
	}

	void PBRPass::CreatePipelines(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight)
	{
		pbrPipeline.SetData(hdrRenderPass, &pbrSetLayoutCache, { swapChainWidth, swapChainHeight });
		pbrPipeline.Create();
	}

	void PBRPass::CreateSetLayoutCaches()
	{
		SetLayoutSummary textureLayout(0);
		textureLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		textureLayout.AddBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		pbrSetLayoutCache.CreateSetLayout(textureLayout, 0);

		SetLayoutSummary projectionLayout(1);
		projectionLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
		pbrSetLayoutCache.CreateSetLayout(projectionLayout, 0);

		SetLayoutSummary volatileLayout(2);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Transform matrix
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // CameraData matrix
		volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
		pbrSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	void PBRPass::CreateDescriptorSets()
	{
		uint32_t pbrSetLayoutCount = pbrSetLayoutCache.GetLayoutCount();
		if (pbrSetLayoutCount != 3)
		{
			LogError("Failed to create pbr descriptor set, invalid set layout count! Expected (%u) vs. actual (%u)", 3, pbrSetLayoutCount);
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

				pbrDescriptorSets[i][j] = TANG::AllocateDescriptorSet(setLayoutOpt.value());
			}
		}
	}

	void PBRPass::CreateUniformBuffers()
	{
		VkDeviceSize transformUBOSize = sizeof(TransformUBO);
		VkDeviceSize viewUBOSize = sizeof(ViewUBO);
		VkDeviceSize projUBOSize = sizeof(ProjUBO);
		VkDeviceSize cameraDataUBOSize = sizeof(CameraDataUBO);

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			transformUBO[i].Create(transformUBOSize);
			transformUBO[i].MapMemory();

			viewUBO[i].Create(viewUBOSize);
			viewUBO[i].MapMemory();

			projUBO[i].Create(projUBOSize);
			projUBO[i].MapMemory();

			cameraDataUBO[i].Create(cameraDataUBOSize);
			cameraDataUBO[i].MapMemory();
		}
	}
}