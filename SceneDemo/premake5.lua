
include "vendor/dependencies.lua"

outputDir = "%{cfg.system}/%{cfg.buildcfg}/%{cfg.architecture}"

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
	
	includedirs
	{
		"%{SceneDemo_IncludeDirs.TANG}",
		"%{SceneDemo_IncludeDirs.glm}",
		"%{SceneDemo_IncludeDirs.assimp}",
		"%{SceneDemo_IncludeDirs.stb_image}",
	}
	
	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"
		warnings "High"
		
		-- Build shaders
		prebuildcommands
		{
			("CALL \"%{wks.location}/TANG/tools/build_shaders.bat\"")
		}
	
	filter "configurations:Debug"
		symbols "On"
		
		links
		{
			"%{SceneDemo_Libraries.assimp_debug}",
			"%{SceneDemo_Libraries.TANG_debug}"
		}
		
		-- Copy assimp debug DLL into exe folder
		postbuildcommands
		{
			("{COPYFILE} \"%{SceneDemo_Libraries.assimp_debug_dll}\" \"%{cfg.targetdir}\"")
		}
	
	filter "configurations:Release"
		optimize "On"
		
		links
		{
			"%{SceneDemo_Libraries.assimp_release}",
			"%{SceneDemo_Libraries.TANG_release}"
		}
		
		-- Copy assimp release DLL into exe folder
		postbuildcommands
		{
			("{COPYFILE} \"%{SceneDemo_Libraries.assimp_release_dll}\" \"%{cfg.targetdir}\"")
		}