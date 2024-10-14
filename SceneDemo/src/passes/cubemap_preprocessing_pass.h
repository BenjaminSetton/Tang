#ifndef CUBEMAP_PREPROCESSING_PASS_H
#define CUBEMAP_PREPROCESSING_PASS_H

#include "TANG/config.h"
#include "TANG/descriptor_set.h"
#include "TANG/framebuffer.h"
#include "TANG/primary_command_buffer.h"
#include "TANG/texture_resource.h"
#include "TANG/uniform_buffer.h"

#include "../pipelines/brdf_convolution_pipeline.h"
#include "../pipelines/cubemap_preprocessing_pipeline.h"
#include "../pipelines/irradiance_sampling_pipeline.h"
#include "../pipelines/prefilter_map_pipeline.h"
#include "../pipelines/skybox_pipeline.h"
#include "../render_passes/brdf_convolution_render_pass.h"
#include "../render_passes/cubemap_preprocessing_render_pass.h"
#include "base_pass.h" // DrawData

class CubemapPreprocessingPass
{
public:

	CubemapPreprocessingPass();
	~CubemapPreprocessingPass();
	CubemapPreprocessingPass(CubemapPreprocessingPass&& other) noexcept;

	CubemapPreprocessingPass(const CubemapPreprocessingPass& other) = delete;
	CubemapPreprocessingPass& operator=(const CubemapPreprocessingPass& other) = delete;

	void Create();

	void DestroyIntermediates();
	void Destroy();

	// Performs and pre-processing necessary for the loaded skybox. 
	// For example, this performs all IBL calculations
	void Draw(TANG::PrimaryCommandBuffer* cmdBuffer, TANG::AssetResources* cubemap, TANG::AssetResources* fullscreenQuad);

	const TANG::TextureResource* GetSkyboxCubemap() const;
	const TANG::TextureResource* GetIrradianceMap() const;
	const TANG::TextureResource* GetPrefilterMap() const;
	const TANG::TextureResource* GetBRDFConvolutionMap() const;

	// Updates the view scope of the prefilter map from PER_MIP_LEVEL to ENTIRE_IMAGE so we can
	// properly sample from all mips. This must be done after we finish rendering to the prefilter map
	// though, so this must be called by the renderer after we wait for the graphics queue
	void UpdatePrefilterMapViewScope();

	VkFence GetFence() const;

private:

	void CreateFramebuffers();
	void CreatePipelines();
	void CreateRenderPasses();
	void CreateSetLayoutCaches();
	void CreateDescriptorSets();
	void CreateUniformBuffers();
	void CreateSyncObjects();

	void InitializeShaderParameters();
	void LoadTextureResources();

	void CalculateSkyboxCubemap(TANG::PrimaryCommandBuffer* cmdBuffer, const TANG::AssetResources* cubemap);
	void CalculateIrradianceMap(TANG::PrimaryCommandBuffer* cmdBuffer, const TANG::AssetResources* cubemap);
	void CalculatePrefilterMap(TANG::PrimaryCommandBuffer* cmdBuffer, const TANG::AssetResources* cubemap);
	void CalculateBRDFConvolution(TANG::PrimaryCommandBuffer* cmdBuffer, const TANG::AssetResources* fullscreenQuad);

	TANG::CubemapPreprocessingPipeline cubemapPreprocessingPipeline;
	TANG::CubemapPreprocessingRenderPass cubemapPreprocessingRenderPass;
	TANG::SetLayoutCache cubemapPreprocessingSetLayoutCache; // Used by cubemap preprocessing + irradiance sampling
	TANG::TextureResource skyboxTexture;
	TANG::TextureResource skyboxCubemap;
	TANG::TextureResource skyboxCubemapMipped; // NOTE - Mips are generated from skyboxCubemap. We can't generate mips on skyboxCubemap directly because it's bound to the framebuffer and consequently the command buffer
	TANG::Framebuffer cubemapPreprocessingFramebuffer;
	TANG::UniformBuffer cubemapPreprocessingViewProjUBO[6];
	TANG::UniformBuffer cubemapPreprocessingCubemapLayerUBO[6];
	TANG::DescriptorSet cubemapPreprocessingDescriptorSets[6];

	TANG::IrradianceSamplingPipeline irradianceSamplingPipeline;
	TANG::DescriptorSet irradianceSamplingDescriptorSets[6];
	TANG::TextureResource irradianceMap;
	TANG::Framebuffer irradianceSamplingFramebuffer;

	TANG::PrefilterMapPipeline prefilterMapPipeline;
	TANG::SetLayoutCache prefilterMapCubemapSetLayoutCache;
	TANG::SetLayoutCache prefilterMapRoughnessSetLayoutCache;
	TANG::UniformBuffer prefilterMapRoughnessUBO[6];
	TANG::DescriptorSet prefilterMapCubemapDescriptorSets[6];
	TANG::DescriptorSet prefilterMapRoughnessDescriptorSets[6];
	TANG::TextureResource prefilterMap;
	TANG::Framebuffer prefilterMapFramebuffers[TANG::CONFIG::PrefilterMapMaxMips]; // One framebuffer per mip level

	TANG::BRDFConvolutionPipeline brdfConvolutionPipeline;
	TANG::BRDFConvolutionRenderPass brdfConvolutionRenderPass;
	TANG::TextureResource brdfConvolutionMap;
	TANG::Framebuffer brdfConvolutionFramebuffer;

	VkFence fence;
	bool wasCreated;
};

#endif