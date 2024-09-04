workspace "TANG"

	architecture "x64"
	startproject "TANG"
	
	configurations
	{
		"Debug",
		"Release"
	}
	
outputDir = "%{cfg.system}/%{cfg.buildcfg}/%{cfg.architecture}"

-- Projects
include "TANG/premake5.lua"
include "SceneDemo/premake5.lua"