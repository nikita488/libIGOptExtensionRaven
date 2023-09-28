IG_ROOT = os.getenv("IG_ROOT")
LIBRAVEN = "C:/libRaven/"

workspace "libIGOptExtensionRaven"
	kind "SharedLib"
	language "C++"
	system "Windows"
	systemversion "latest"
	characterset "MBCS"
	location "build"
	configurations { "Debug", "Release" }
	targetdir "bin/%{cfg.buildcfg}"
	
	filter "configurations:Debug"
		libdirs { path.join(IG_ROOT, "DirectX9/libdbg") }
		libdirs { path.join(LIBRAVEN, "libdbg") }
		defines { "WIN32", "_DEBUG", "_WINDOWS", "_USRDLL", "LIBIGOPTEXTENSIONRAVEN_EXPORT", "LIBIGOPTEXTENSIONRAVEN_DYNAMIC", "IG_GFX_DX9", "IG_TARGET_WIN32", "IG_TARGET_TYPE_WIN32", "IG_ALCHEMY_DLL", "IG_DEBUG" }
		symbols "On"
		postbuildcommands "copy /y \"$(TargetDir)\" \"..\\libdbg\\\""
	filter {}
	
	filter "configurations:Release"
		libdirs { path.join(IG_ROOT, "DirectX9/lib") }
		libdirs { path.join(LIBRAVEN, "lib") }
		defines { "WIN32", "NDEBUG", "_WINDOWS", "_USRDLL", "LIBIGOPTEXTENSIONRAVEN_EXPORT", "LIBIGOPTEXTENSIONRAVEN_DYNAMIC", "IG_GFX_DX9", "IG_TARGET_WIN32", "IG_TARGET_TYPE_WIN32", "IG_ALCHEMY_DLL" }
		optimize "On"
		postbuildcommands "copy /y \"$(TargetDir)\" \"..\\lib\\\""
	filter {}
	
	includedirs { path.join(IG_ROOT, "include") }
	includedirs { path.join(LIBRAVEN, "include") }
	includedirs { path.join(IG_ROOT, "sources/exporters/common/headers") }
	
	links { "libIGCore", "libIGMath", "libIGAttrs", "libIGSg", "libIGGfx", "libIGOpt", "libRaven", "libIGExportCommon" }
   
	files { "src/*.*" }
   
project "libIGOptExtensionRaven"