#pragma once

#include "core.h"

// per frame telemetry info
struct Telemetry
{
	// rendering
	u32 gfx_polygonCount; // ~Incomplete

	// memory
	u32 mem_frameArenaUsage;
};

extern Telemetry telemetry;
void ResetTelemetry();

