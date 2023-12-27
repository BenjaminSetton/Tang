workspace "TANG"

	architecture "x64"
	startproject "TANG"
	
	configurations
	{
		"Debug",
		"Release"
	}
	
outputDir = "%{cfg.system}/%{cfg.buildcfg}/%{cfg.architecture}"

include "vendor/dependencies.lua"
	
project "TANG"
	location "out"
	language "C++"
	kind "ConsoleApp"
	
	targetdir ("out/bin/" .. outputDir)
	objdir ("out/obj/" .. outputDir)
	
	files
	{
		"src/**.h",
		"src/**.cpp",
		"src/**.vert",
		"src/**.geo",
		"src/**.frag",
	}
	
	includedirs
	{
		"%{IncludeDirs.assimp}",
		"%{IncludeDirs.glfw}",
		"%{IncludeDirs.glm}",
		"%{IncludeDirs.stb_image}",
		"%{IncludeDirs.vulkan}",
	}
	
	links
	{
		"%{Libraries.vulkan}"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"
		warnings "High"
		
		-- Build shaders
		prebuildcommands
		{
			("CALL \"%{wks.location}/utils/compile_shaders.bat\"")
		}
	
	filter "configurations:Debug"
		defines "TNG_DEBUG"
		symbols "On"
		
		links
		{
			"%{Libraries.assimp_debug}",
			"%{Libraries.glfw_debug}",
		}
		
		-- Copy assimp debug DLL into exe folder
		postbuildcommands
		{
			("{COPYFILE} \"%{Libraries.assimp_debug_dll}\" \"%{cfg.targetdir}\"")
		}
	
	filter "configurations:Release"
		defines "TNG_NDEBUG"
		optimize "On"
		
		links
		{
			"%{Libraries.assimp_release}",
			"%{Libraries.glfw_release}",
		}
		
		-- Copy assimp release DLL into exe folder
		postbuildcommands
		{
			("{COPYFILE} \"%{Libraries.assimp_release_dll}\" \"%{cfg.targetdir}\"")
		}
		
		