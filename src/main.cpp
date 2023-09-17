
#include <string>

#include "tang.h"
#include "utils/sanity_check.h"

static constexpr uint32_t NUM_ASSETS = 1;
static std::string assets[NUM_ASSETS] =
{
    "../src/data/assets/sample/torus_smooth.fbx"
};

int main(uint32_t argc, const char** argv) 
{
    UNUSED(argc);
    UNUSED(argv);

    TANG::Initialize();
    TANG::LoadAsset(assets[0].c_str());
    while (!TANG::WindowShouldClose())
    {
        TANG::Update(0);
    }

    TANG::Shutdown();

    return EXIT_SUCCESS;
}
