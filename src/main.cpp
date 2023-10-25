
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <time.h>
#include <vector>

#include "tang.h"
#include "utils/sanity_check.h"
#include "utils/logger.h"

struct MyAsset
{
	MyAsset(std::string _name) : name(_name), uuid(TANG::INVALID_UUID)
	{
	}

    MyAsset(std::string _name, TANG::UUID _uuid) : name(_name), uuid(_uuid)
    {
    }

    std::string name;
    TANG::UUID uuid;
    float pos[3]   = {0.0f, 0.0f, 0.0f};
    float rot[3]   = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
};

static std::vector<std::string> assetNames =
{
    "../src/data/assets/sample/axe/scene.gltf",
};

int RandomRangeInt(int min, int max)
{
    return rand() % (max - min) + min;
}

float RandomRangeFloat(float min, float max)
{
    return (rand() / static_cast<float>(RAND_MAX)) * (max - min) + min;
}

int main(uint32_t argc, const char** argv) 
{
    UNUSED(argc);
    UNUSED(argv);

    // Seed rand()
    srand(static_cast<unsigned int>(time(NULL)));

    TANG::Initialize();

    std::vector<MyAsset> assets;

    // Load all the assets
    for (auto& assetName : assetNames)
    {
        TANG::UUID id = TANG::LoadAsset(assetName.c_str());
        if (id != TANG::INVALID_UUID)
        {
            MyAsset asset(assetName, id);

            float scale = 0.005f;
            asset.scale[0] = scale;
            asset.scale[1] = scale;
            asset.scale[2] = scale;

            TANG::UpdateAssetTransform(id, asset.pos, asset.rot, asset.scale);

            assets.push_back(asset);
        }
    }

    // Set the camera's velocity
    TANG::SetCameraSpeed(10.0f);
    TANG::SetCameraSensitivity(5.0f);

    float elapsedTime = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (!TANG::WindowShouldClose())
    {
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - startTime).count();
        elapsedTime += dt;

        startTime = std::chrono::high_resolution_clock::now();

		for (auto& asset : assets)
		{
            float rot[3] = {asset.rot[0], 33.0f * elapsedTime, asset.rot[2]};

            TANG::UUID id = asset.uuid;

            TANG::ShowAsset(id);
            TANG::UpdateAssetRotation(id, rot, true);
		}
        
        TANG::Update(dt);

        TANG::Draw();
    }

    TANG::Shutdown();

    return EXIT_SUCCESS;
}
