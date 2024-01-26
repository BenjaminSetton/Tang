#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace TANG
{
	namespace CONFIG
	{
		static constexpr uint32_t WindowWidth = 1920;
		static constexpr uint32_t WindowHeight = 1080;

		static const std::string SkyboxCubeMeshFilePath = "../src/data/assets/cube.fbx";
		static const std::string SkyboxTextureFilePath = "../src/data/textures/cubemaps/skybox_sunny.hdr";
		static const uint32_t SkyboxCubemapResolutionSize = 512;
		static const uint32_t IrradianceMapSize = 32;
		static const uint32_t PrefilterMapSize = 128;
		static const uint32_t PrefilterMapMaxMips = 5; // Ensure 2 to the power of this value does not exceed PrefilterMapSize above!

		static constexpr uint32_t MaxFramesInFlight = 2;
		static constexpr uint32_t MaxAssetCount = 100;

		static const std::string MaterialTexturesFilePath = "../src/data/textures/";

		static const std::string FullscreenQuadMeshFilePath = "../src/data/assets/fullscreen_quad.fbx";
	}
}

#endif