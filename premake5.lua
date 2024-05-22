workspace "hexray-gpu"
	architecture "x64"
	startproject "hexray-gpu"

	configurations 
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	outputdir = "%{cfg.buildcfg}"

project "hexray-gpu"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	systemversion "latest"
	staticruntime "on"
	characterset("ASCII")

	targetdir("%{wks.location}/bin/" .. outputdir)
	objdir("%{wks.location}/tmp/" .. outputdir)

	files
	{
		"%{wks.location}/src/**.cpp",
		"%{wks.location}/src/**.h"
	}

	includedirs
	{
		"%{wks.location}/src",
		"%{wks.location}/extern/spdlog/include",
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

		defines
		{
			"HEXRAY_DEBUG",
			"_DEBUG"
		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

		defines
		{
			"HEXRAY_RELEASE",
			"NDEBUG"
		}