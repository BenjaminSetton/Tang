#ifndef SKYBOX_PASS_H
#define SKYBOX_PASS_H

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/skybox_pipeline.h"
#include "base_pass.h"
#include "../config.h"

namespace TANG
{
	class SkyboxPass : public BasePass
	{
	public:

		SkyboxPass();
		~SkyboxPass();
		SkyboxPass(SkyboxPass&& other) noexcept;

		SkyboxPass(const SkyboxPass& other) = delete;
		SkyboxPass& operator=(const SkyboxPass& other) = delete;

		void Create(const DescriptorPool& descriptorPool);
		void Destroy();

		void Draw(uint32_t currentFrame, const DrawData& data) override;

	private:

		void CreatePipelines() override;
		void CreateSetLayoutCaches() override;
		void CreateUniformBuffers() override;

		SkyboxPipeline skyboxPipeline;
		SetLayoutCache skyboxSetLayoutCache;
		std::array<std::array<DescriptorSet, 3>, CONFIG::MaxFramesInFlight> skyboxDescriptorSets;

	};
}

#endif