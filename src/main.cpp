
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
    "../src/data/assets/sample/suzanne_smooth.fbx",
    "../src/data/assets/sample/torus.fbx",
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

    for (auto& assetName : assetNames)
    {
        TANG::UUID id = TANG::LoadAsset(assetName.c_str());
        if (id != TANG::INVALID_UUID)
        {
            MyAsset asset(assetName, id);

            // Give every asset some random transforms for testing
            asset.pos[0] = RandomRangeFloat(-5.0f, 5.0f);
            asset.pos[2] = RandomRangeFloat(-5.0f, 5.0f);

            asset.rot[0] = RandomRangeFloat(-90.0f, 90.0f);

            float randScale = RandomRangeFloat(0.5f, 1.5f);
            asset.scale[0] = randScale;
            asset.scale[1] = randScale;
            asset.scale[2] = randScale;

            TANG::UpdateAssetTransform(id, asset.pos, asset.rot, asset.scale);

            assets.push_back(asset);
        }
    }

    //uint64_t frameCount = 0;
    float elapsedTime = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (!TANG::WindowShouldClose())
    {
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - startTime).count();
        elapsedTime += dt;

        startTime = std::chrono::high_resolution_clock::now();

		for (auto& asset : assets)
		{
            float rot[3] = {asset.rot[0], 90.0f * elapsedTime, asset.rot[2]};

            TANG::UUID id = asset.uuid;

            TANG::ShowAsset(id);
            TANG::UpdateAssetRotation(id, rot, true);
		}
        
        TANG::Update(&dt);

        TANG::Draw();
    }

    TANG::Shutdown();

    return EXIT_SUCCESS;
}
