workspace "project_new_winds"
    architecture "x64"
    configurations { "debug", "release", "dist" }

outputdir = "%{cfg.system}_%{cfg.buildcfg}_%{cfg.architecture}"

project "nw"
    location "game"
    kind "ConsoleApp"
    language "C++"

    targetdir ("build/" .. outputdir)
    objdir ("build/obj/" .. outputdir .. "/%{prj.name}")
    debugdir ("res/")
    cppdialect ("C++17") -- This is temporary, we might switch back

    files { "src/**.h", "src/**.cpp" }

	libdirs { "lib/%{cfg.system}/" }

	includedirs	
	{
		"src/",
		"src/glad/",
        "include/",
		"include/physx/" -- may be a bit janky?
    }

	defines
	{
		"PX_PHYSX_STATIC_LIB"
	}

    filter "system:windows"
        systemversion "latest"

		-- because windows is bad
		staticruntime "on"
		runtime "Release"

        defines
        {
            "PLATFORM_WINDOWS"
        }        
        
        links
        {
            "SDL2",
            "d3d11",
            "dxgi",
            "vulkan-1",
			
			"shaderc_shared",
			
			
			"PhysX_64",
			"PhysXCooking_64",
			"PhysXCommon_64",
			"PhysXFoundation_64",
			"PhysXExtensions_static_64",
			"PhysXPvdSDK_static_64",
        }

        postbuildcommands
        {
            ("{COPY} ..\\exports\\windows\\* \"..\\build\\" .. outputdir .. "\\\"")
        }

    filter "system:linux"
        defines
        {
            "PLATFORM_LINUX"
        }


        links
        {
            "SDL2",
		    "vulkan",
			"pthread",

			"shaderc_combined",
			
			"pulse", -- audio
			"pulse-simple",

			"PhysX",
			"PhysXPvdSDK",
			"PhysXExtensions",
			"PhysXCooking",
			"PhysXCommon",
			"PhysXFoundation",
			
			"dl",
        }

        postbuildcommands
        {
            ("mkdir -p ../build/" .. outputdir .. "/" ..  "bin" .. "/" .. "&& cp -r ../exports/linux/* ../build/" .. outputdir .. "/")
        }


    filter "configurations:debug"
        defines { "BUILD_DEBUG" }
        symbols "On"
        optimize "Off"
		libdirs { "lib/%{cfg.system}/debug" }

    filter "configurations:release"
        defines { "BUILD_RELEASE" }
        symbols "On"
        optimize "On"
		libdirs { "lib/%{cfg.system}/release" }

    filter "configurations:dist"
        defines { "BUILD_RELEASE", "BUILD_DIST" } -- Maybe a bad idea?
        symbols "Off"
        optimize "On"
		libdirs { "lib/%{cfg.system}/release" }

        
    filter "system:macosx"
        defines
        {
            "PLATFORM_MACOS"
        }

        frameworkdirs 
        {
            "lib/macos/"
        }

        libdirs
        {
            "lib/macos/"
        }

        links 
        {
            "SDL2.framework",
            
			"shaderc_combined",
        }

        --postbuildcommands
    -- {
        -- ("mkdir -p ../build/" .. outputdir .. "/" ..  "bin" .. "/" .. "&& cp -r ../exports/macos/* ../build/" .. outputdir .. "/")
    -- }