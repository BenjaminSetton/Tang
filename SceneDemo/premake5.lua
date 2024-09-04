project "SceneDemo"
	location "out"
	language "C++"
	kind "ConsoleApp"
	
	targetdir ("out/bin/" .. outputDir)
	objdir ("out/obj/" .. outputDir)
	
	files
	{
		"src/**.h",
		"src/**.cpp",
		"src/shaders/**.vert",
		"src/shaders/**.geo",
		"src/shaders/**.frag",
		"src/shaders/**.comp",
		"src/shaders/**.glsl"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"
		warnings "High"
	
	filter "configurations:Debug"
		symbols "On"
	
	filter "configurations:Release"
		optimize "On"