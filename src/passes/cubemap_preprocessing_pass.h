#ifndef CUBEMAP_PREPROCESSING_PASS_H
#define CUBEMAP_PREPROCESSING_PASS_H

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../framebuffer.h"
#include "../pipelines/cubemap_preprocessing_pipeline.h"
#include "../pipelines/irradiance_sampling_pipeline.h"
#include "../pipelines/prefilter_map_pipeline.h"
#include "../pipelines/skybox_pipeline.h"
#include "../render_passes/cubemap_preprocessing_render_pass.h"
#include "../texture_resource.h"
#include "base_pass.h"
#include "../config.h"

namespace TANG
{
	class CubemapPreprocessingPass : public BasePass
	{
	public:

		CubemapPreprocessingPass();
		~CubemapPreprocessingPass();
		CubemapPreprocessingPass(CubemapPreprocessingPass&& other) noexcept;

		CubemapPreprocessingPass(const CubemapPreprocessingPass& other) = delete;
		CubemapPreprocessingPass& operator=(const CubemapPreprocessingPass& other) = delete;

		void SetData(const DescriptorPool* descriptorPool, VkExtent2D swapChainExtent);

		void Create() override;

		void DestroyIntermediates();
		void Destroy() override;

		void LoadTextureResources();

		// Performs and pre-processing necessary for the loaded skybox. 
		// For example, this performs all IBL calculations
		void Preprocess(PrimaryCommandBuffer* cmdBuffer, AssetResources* asset);

		// No-op, all the work is done during Preprocessing()
		void Draw(uint32_t currentFrame, const DrawData& data) override;

		const TextureResource* GetSkyboxCubemap() const;
		const TextureResource* GetIrradianceMap() const;

	private:

		void CreateFramebuffers() override;
		void CreatePipelines() override;
		void CreateRenderPasses() override;
		void CreateSetLayoutCaches() override;
		void CreateDescriptorSets() override;
		void CreateUniformBuffers() override;
		void CreateSyncObjects() override;

		void InitializeShaderParameters();

		void CalculateSkyboxCubemap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset);
		void CalculateIrradianceMap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset);
		void CalculatePrefilterMap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset);

		void ResetBorrowedData() override;

		CubemapPreprocessingPipeline cubemapPreprocessingPipeline;
		CubemapPreprocessingRenderPass cubemapPreprocessingRenderPass;
		SetLayoutCache cubemapPreprocessingSetLayoutCache; // Used by cubemap preprocessing + irradiance sampling
		TextureResource skyboxTexture;
		TextureResource skyboxCubemap;
		Framebuffer cubemapPreprocessingFramebuffer;
		UniformBuffer cubemapPreprocessingViewProjUBO[6];
		UniformBuffer cubemapPreprocessingCubemapLayerUBO[6];
		DescriptorSet cubemapPreprocessingDescriptorSets[6];

		IrradianceSamplingPipeline irradianceSamplingPipeline;
		DescriptorSet irradianceSamplingDescriptorSets[6];
		TextureResource irradianceMap;
		Framebuffer irradianceSamplingFramebuffer;

		PrefilterMapPipeline prefilterMapPipeline;
		SetLayoutCache prefilterMapSetLayoutCache;
		UniformBuffer prefilterMapRoughnessUBO[6];
		DescriptorSet prefilterMapDescriptorSets[6];
		TextureResource prefilterMap;
		Framebuffer prefilterMapFramebuffer; // Represents one cubemap mip level

		struct
		{
			const DescriptorPool* descriptorPool;
			VkExtent2D swapChainExtent;
		} borrowedData;
	};
}

#endif