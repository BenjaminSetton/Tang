#ifndef CUBEMAP_PREPROCESSING_PASS_H
#define CUBEMAP_PREPROCESSING_PASS_H

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../framebuffer.h"
#include "../pipelines/cubemap_preprocessing_pipeline.h"
#include "../pipelines/irradiance_sampling_pipeline.h"
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

		void Create(const DescriptorPool& descriptorPool);

		void DestroyIntermediates();
		void Destroy();

		void LoadTextureResources();

		// Performs and pre-processing necessary for the loaded skybox. 
		// For example, this performs all IBL calculations
		void Preprocess(PrimaryCommandBuffer* cmdBuffer, AssetResources* asset);

		void Draw(uint32_t currentFrame, const DrawData& data) override;

		const TextureResource* GetSkyboxCubemap() const;
		const TextureResource* GetIrradianceMap() const;

	private:

		void CreateFramebuffers() override;
		void CreatePipelines() override;
		void CreateRenderPasses() override;
		void CreateSetLayoutCaches() override;
		void CreateDescriptorSets(const DescriptorPool& descriptorPool) override;
		void CreateUniformBuffers() override;
		void CreateSyncObjects() override;

		void InitializeShaderParameters();

		void CalculateSkyboxCubemap(PrimaryCommandBuffer* cmdBuffer, AssetResources* asset);
		void CalculateIrradianceMap(PrimaryCommandBuffer* cmdBuffer, AssetResources* asset);

		CubemapPreprocessingPipeline cubemapPreprocessingPipeline;
		CubemapPreprocessingRenderPass cubemapPreprocessingRenderPass;
		SetLayoutCache cubemapPreprocessingSetLayoutCache;
		TextureResource skyboxTexture;
		TextureResource skyboxCubemap;
		Framebuffer cubemapPreprocessingFramebuffer;
		UniformBuffer cubemapPreprocessingViewProjUBO[6];
		UniformBuffer cubemapPreprocessingCubemapLayerUBO[6];
		DescriptorSet cubemapPreprocessingDescriptorSets[6];

		IrradianceSamplingPipeline irradianceSamplingPipeline;
		SetLayoutCache irradianceSamplingSetLayoutCache;
		DescriptorSet irradianceSamplingsDescriptorSets[6];
		TextureResource irradianceMap;
		Framebuffer irradianceSamplingFramebuffer;
	};
}

#endif