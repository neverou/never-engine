#!/bin/bash
echo ""
echo "-------[[ never build ]]-------"
echo "we're building never engine!! ^^"



# Determine the platform
Platform=""
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	Platform="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
	Platform="macos"
elif [[ "$OSTYPE" == "msys" ]]; then
	Platform="windows"
else
	echo "uh oh! unknown OS ($OSTYPE)"
	exit -1
fi

ExeName="new_winds"

if [[ "$Platform" == "windows" ]]; then
	ActualExeName="$ExeName.exe"
else
	ActualExeName="$ExeName"
fi



if [ -z "$Config" ]; then
	Config="debug"
fi

PlatformDefines=""
PlatformLinks=""
ConfigArgs=""

if [[ "$Config" == "debug" ]]; then
	ConfigArgs="-DBUILD_DEBUG -g -O0"
elif [[ "$Config" == "release" ]]; then
	ConfigArgs="-DBUILD_RELEASE -g -Ofast"
elif [[ "$Config" == "dist" ]]; then # Release but without debug symbols
	ConfigArgs="-DBUILD_RELEASE -DBUILD_DIST -Ofast"
else
	echo "uh oh! unknown build config ($Config)"
	exit -1
fi

if [[ "$Platform" == "linux" ]]; then
    PlatformDefines="-DPLATFORM_LINUX"
	PlatformLinks="-lvulkan -lshaderc_combined -lpthread -lpulse -lpulse-simple -ldl -lPhysX -lPhysXPvdSDK -lPhysXExtensions -lPhysXCooking -lPhysXCommon -lPhysXFoundation"
elif [[ "$Platform" == "macos" ]]; then
    PlatformDefines="-DPLATFORM_MACOS"
	PlatformLinks=""
	
	echo "Mac is not implemented yet!"
	exit -1
elif [[ "$Platform" == "windows" ]]; then
	PlatformDefines="-DPLATFORM_WINDOWS"
	PlatformLinks="-lvulkan-1 -ldxgi -ld3d11 -lshaderc_shared -lxaudio2 -lPhysX_64 -lPhysXPvdSDK_static_64 -lPhysXExtensions_static_64 -lPhysXCooking_64 -lPhysXCommon_64 -lPhysXFoundation_64 -lole32"
else
	echo "invalid OS state, this error really shouldn't be possible!! probably indicates a bug!"
	exit -1
fi


Compiler="clang++ -m64" # 64 bit

Include="-Iinclude -Iinclude/physx -Isrc -Isrc/glad"
SourceFiles="src/game.cpp src/audio.cpp"

Links="-lSDL2 "


# Warning Supressions (Because we will eventually enable -Werror so warnings will be errors)
# We dont care about switch statements because we intentionally use them like if long statements
# We dont care about unused variables and stuff
WarningSupressions="-Wall -Wno-switch -Wno-unused-variable -Wno-unused-local-typedef -Wno-char-subscripts -Wno-unused-function" # -Werror


OutDir="bin/$Platform-$Config"
Library="-Llib/$Platform/ -Llib/$Platform/$Config"
ExportFolder="exports/$Platform"

echo "compiling with config=$Config platform=$Platform"

mkdir -p "$OutDir"
time $Compiler $SourceFiles $Include $Library $Links $PlatformLinks $PlatformDefines $ConfigArgs -o "$OutDir/$ActualExeName" $WarningSupressions
cp -r "$ExportFolder/." "$OutDir"