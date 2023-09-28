/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   TANG library
// 
//   Benjamin Setton
//   2023
// 
//   TANG is a simple Vulkan rendering library meant to kick-start the rendering process for your own projects! 
//   The API is written to make it fast and easy to draw something on the screen.
// 
//   Before using any of the TANG function API calls, make sure to initialize the library by calling:
//      TANG::Initialize();
//   and clean-up the libraries' resources at the end of your program by calling
//      TANG::Shutdown();
// 
//   The rest of the API calls are structured in two main components: state calls and update calls.
//   
//   STATE CALLS  - Meant to be called once to set or load a state for a non-trivial period
//                  of time. Examples include loading assets or setting the preferred
//                  renderer state
//   
//   UPDATE CALLS - Meant to be called on a per-frame basis. The effect of these functions will only
//                  last a single frame, so as soon as they're not called anymore the effect will not
//                  be visible on the screen. Examples include drawing assets or simple polygons and
//                  setting the camera transform.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace TANG
{
	void Initialize();
	void Update(float* deltaTime);
	void Draw();
	void Shutdown();

	// STATE CALLS
	bool WindowShouldClose();

	bool LoadAsset(const char* name);

	// UPDATE CALLS
	void ShowAsset(const char* name);

}