#ifndef CUBEMAP_PREPROCESSING_PASS_H
#define CUBEMAP_PREPROCESSING_PASS_H

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../framebuffer.h"
#include "../pipelines/brdf_convolution_pipeline.h"
#include "../pipelines/cubemap_preprocessing_pipeline.h"
#include "../pipelines/irradiance_sampling_pipeline.h"
#include "../pipelines/prefilter_map_pipeline.h"
#include "../pipelines/skybox_pipeline.h"
#include "../render_passes/brdf_convolution_render_pass.h"
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
		void Preprocess(PrimaryCommandBuffer* cmdBuffer, AssetResources* cubemap, AssetResources* fullscreenQuad);

		// No-op, all the work is done during Preprocessing()
		void Draw(uint32_t currentFrame, const DrawData& data) override;

		const TextureResource* GetSkyboxCubemap() const;
		const TextureResource* GetIrradianceMap() const;
		const TextureResource* GetPrefilterMap() const;
		const TextureResource* GetBRDFConvolutionMap() const;

		// Updates the view scope of the prefilter map from PER_MIP_LEVEL to ENTIRE_IMAGE so we can
		// properly sample from all mips. This must be done after we finish rendering to the prefilter map
		// though, so this must be called by the renderer after we wait for the graphics queue
		void UpdatePrefilterMapViewScope();

	private:

		void CreateFramebuffers() override;
		void CreatePipelines() override;
		void CreateRenderPasses() override;
		void CreateSetLayoutCaches() override;
		void CreateDescriptorSets() override;
		void CreateUniformBuffers() override;
		void CreateSyncObjects() override;

		void InitializeShaderParameters();

		void CalculateSkyboxCubemap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* cubemap);
		void CalculateIrradianceMap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* cubemap);
		void CalculatePrefilterMap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* cubemap);
		void CalculateBRDFConvolution(PrimaryCommandBuffer* cmdBuffer, const AssetResources* fullscreenQuad);

		void ResetBorrowedData() override;

		CubemapPreprocessingPipeline cubemapPreprocessingPipeline;
		CubemapPreprocessingRenderPass cubemapPreprocessingRenderPass;
		SetLayoutCache cubemapPreprocessingSetLayoutCache; // Used by cubemap preprocessing + irradiance sampling
		TextureResource skyboxTexture;
		TextureResource skyboxCubemap;
		TextureResource skyboxCubemapMipped; // NOTE - Mips are generated from skyboxCubemap. We can't generate mips on skyboxCubemap directly because it's bound to the framebuffer and consequently the command buffer
		Framebuffer cubemapPreprocessingFramebuffer;
		UniformBuffer cubemapPreprocessingViewProjUBO[6];
		UniformBuffer cubemapPreprocessingCubemapLayerUBO[6];
		DescriptorSet cubemapPreprocessingDescriptorSets[6];

		IrradianceSamplingPipeline irradianceSamplingPipeline;
		DescriptorSet irradianceSamplingDescriptorSets[6];
		TextureResource irradianceMap;
		Framebuffer irradianceSamplingFramebuffer;

		PrefilterMapPipeline prefilterMapPipeline;
		SetLayoutCache prefilterMapCubemapSetLayoutCache;
		SetLayoutCache prefilterMapRoughnessSetLayoutCache;
		UniformBuffer prefilterMapRoughnessUBO[6];
		DescriptorSet prefilterMapCubemapDescriptorSets[6];
		DescriptorSet prefilterMapRoughnessDescriptorSets[6];
		TextureResource prefilterMap;
		Framebuffer prefilterMapFramebuffers[CONFIG::PrefilterMapMaxMips]; // One framebuffer per mip level

		BRDFConvolutionPipeline brdfConvolutionPipeline;
		BRDFConvolutionRenderPass brdfConvolutionRenderPass;
		TextureResource brdfConvolutionMap;
		Framebuffer brdfConvolutionFramebuffer;

		struct
		{
			const DescriptorPool* descriptorPool;
			VkExtent2D swapChainExtent;
		} borrowedData;
	};
}

#endif