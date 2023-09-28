
#include <iostream>
#include <string>
#include <vector>

#include "tang.h"
#include "utils/sanity_check.h"
#include "utils/logger.h"

static std::vector<std::string> assets =
{
    "../src/data/assets/sample/suzanne_smooth.fbx",
};

int main(uint32_t argc, const char** argv) 
{
    UNUSED(argc);
    UNUSED(argv);

    TANG::Initialize();

    for (auto& assetName : assets)
    {
        TANG::LoadAsset(assetName.c_str());
    }

    uint64_t frameCount = 0;
    while (!TANG::WindowShouldClose())
    {
		for (auto& assetName : assets)
		{
            TANG::ShowAsset(assetName.c_str());
		}
        
        TANG::Update(0);

        TANG::Draw();
    }

    TANG::Shutdown();

    return EXIT_SUCCESS;
}
