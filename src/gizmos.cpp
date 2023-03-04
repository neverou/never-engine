#include "gizmos.h"

#include "std.h"
#include "game.h"

#include "resource.h"

intern struct 
{
	bool init = false;

	DynArray<GizmoVertex> vertices;
	RendererHandle drawBuffer;
	
	struct 
	{
		Mat4 projectionMatrix;
		Mat4 viewMatrix;
	} drawConsts;
	RendererHandle drawConstBuffer;

	RendererHandle shader;
} gizmos;

// ~Temp
const u64 Gizmo_Max_Vertices = 50000;

void ResetGizmos()
{
	if (!gizmos.init)
	{	
		Renderer* renderer = game->renderer;

		gizmos.init = true;

		// we dont destroy these anywhere, we expect the OS to free it on exit 
		// maybe ~Hack?
		gizmos.vertices   = MakeDynArray<GizmoVertex>(0, NULL, false);
		gizmos.drawBuffer = renderer->CreateBuffer(NULL, sizeof(GizmoVertex) * Gizmo_Max_Vertices, RENDERBUFFER_FLAGS_VERTEX | RENDERBUFFER_FLAGS_USAGE_STREAM);
		if (!gizmos.drawBuffer)
			LogWarn("[gizmos] failed to allocate gizmos buffer! gizmos will not render");

		gizmos.drawConstBuffer = renderer->CreateBuffer(NULL, sizeof(GizmoVertex) * Gizmo_Max_Vertices, RENDERBUFFER_FLAGS_CONSTANT | RENDERBUFFER_FLAGS_USAGE_STREAM);

		auto* shader = LoadShader("shaders/gizmos.shader");
		if (shader)
			gizmos.shader = shader->handle;
		else
			gizmos.shader = 0;

		if (!gizmos.shader)
			LogWarn("[gizmos] unable to load gizmo shader, gizmos will not render");
	}


	ArrayClear(&gizmos.vertices);
}

void DrawGizmos(Mat4 projectionMatrix, Mat4 viewMatrix)
{

	if (gizmos.vertices.size > 0)
	{
		Renderer* renderer = game->renderer;

		gizmos.drawConsts.projectionMatrix = projectionMatrix;
		gizmos.drawConsts.viewMatrix 	   = viewMatrix;

		void* constants = renderer->MapBuffer(gizmos.drawConstBuffer);
		Memcpy(constants, &gizmos.drawConsts, sizeof(gizmos.drawConsts));
		renderer->UnmapBuffer(gizmos.drawConstBuffer);

		void* drawData = renderer->MapBuffer(gizmos.drawBuffer);
		Memcpy(drawData, gizmos.vertices.data, sizeof(GizmoVertex) * gizmos.vertices.size);
		renderer->UnmapBuffer(gizmos.drawBuffer);

		if (!gizmos.shader) return;
		renderer->CmdSetShader(gizmos.shader);

		if (!gizmos.drawBuffer) return;

		FixArray<RendererHandle, 1> cb;
		cb[0] = gizmos.drawConstBuffer;
		renderer->CmdSetConstantBuffers(gizmos.shader, 0, &cb);


		FixArray<RendererHandle, 1> vb;
		vb[0] = gizmos.drawBuffer;
		renderer->CmdBindVertexBuffers(0, &vb, NULL);
		renderer->CmdDrawIndexed(gizmos.vertices.size, 0, 0, 1, 0); // ~FixMe use CmdDraw instead of CmdDrawIndexed
	}
}

void PushGizmoVertex(GizmoVertex vertex)
{
	ArrayAdd(&gizmos.vertices, vertex);
}



void GizmoArrow(Vec3 from, Vec3 to, float neckRadius, float headRadius, float headSegment, Vec3 color)
{
	Vec3 dir = Normalize(Sub(to, from));
	
	Vec3 tmpBitan = v3(0, 1, 0);
	if (Dot(dir, tmpBitan) == 1)
		tmpBitan = v3(1, 0, 0);
	
	Vec3 tangent   = Normalize(Cross(dir, tmpBitan));
	Vec3 cotangent = Cross(dir, tangent);

	float length = Length(Sub(from, to));

	Vec3 headStart = Add(from, Mul(dir, v3(length - headSegment)));


	constexpr int Arrow_Segments = 32;

	// neck
	for (int i = 0; i < Arrow_Segments; i++)
	{
		float u0 = Cos(i / 32.0 * 360)  	 * neckRadius;
		float v0 = Sin(i / 32.0 * 360)  	 * neckRadius;
		
		float u1 = Cos((i + 1) / 32.0 * 360) * neckRadius;
		float v1 = Sin((i + 1) / 32.0 * 360) * neckRadius;
		
		Vec3 radial0 = Add(Mul(tangent, v3(u0)), Mul(cotangent, v3(v0)));
		Vec3 radial1 = Add(Mul(tangent, v3(u1)), Mul(cotangent, v3(v1)));

		ArrayAdd(&gizmos.vertices, { Add(from, 		  radial0), color });
		ArrayAdd(&gizmos.vertices, { Add(from, 		  radial1), color });
		ArrayAdd(&gizmos.vertices, { Add(headStart,   radial0), color });

		ArrayAdd(&gizmos.vertices, { Add(headStart,   radial1), color });
		ArrayAdd(&gizmos.vertices, { Add(headStart,   radial0), color });
		ArrayAdd(&gizmos.vertices, { Add(from, 	      radial1), color });
	}

	// head
	for (int i = 0; i < Arrow_Segments; i++)
	{
		float u0 = Cos(i / 32.0 * 360)  	  * headRadius;
		float v0 = Sin(i / 32.0 * 360)  	  * headRadius;
		
		float u1 = Cos((i + 1) / 32.0 * 360)  * headRadius;
		float v1 = Sin((i + 1) / 32.0 * 360)  * headRadius;
		
		Vec3 radial0 = Add(Mul(tangent, v3(u0)), Mul(cotangent, v3(v0)));
		Vec3 radial1 = Add(Mul(tangent, v3(u1)), Mul(cotangent, v3(v1)));

		ArrayAdd(&gizmos.vertices, { Add(headStart, radial0), color });
		ArrayAdd(&gizmos.vertices, { Add(headStart, radial1), color });
		ArrayAdd(&gizmos.vertices, { to, color });
	}
}


void GizmoHandle(Vec3 from, Vec3 to, float neckRadius, float headRadius, Vec3 color)
{
	Vec3 dir = Normalize(Sub(to, from));
	
	Vec3 tmpBitan = v3(0, 1, 0);
	if (Dot(dir, tmpBitan) == 1)
		tmpBitan = v3(1, 0, 0);
	
	Vec3 tangent   = Normalize(Cross(dir, tmpBitan));
	Vec3 cotangent = Cross(dir, tangent);

	float length = Length(Sub(from, to));

	Vec3 headStart = Add(from, Mul(dir, v3(length - headRadius)));

	constexpr int Handle_Segments = 32;

	// neck
	for (int i = 0; i < Handle_Segments; i++)
	{
		float u0 = Cos(i / 32.0 * 360)  	 * neckRadius;
		float v0 = Sin(i / 32.0 * 360)  	 * neckRadius;
		
		float u1 = Cos((i + 1) / 32.0 * 360) * neckRadius;
		float v1 = Sin((i + 1) / 32.0 * 360) * neckRadius;
		
		Vec3 radial0 = Add(Mul(tangent, v3(u0)), Mul(cotangent, v3(v0)));
		Vec3 radial1 = Add(Mul(tangent, v3(u1)), Mul(cotangent, v3(v1)));

		ArrayAdd(&gizmos.vertices, { Add(from, 		  radial0), color });
		ArrayAdd(&gizmos.vertices, { Add(from, 		  radial1), color });
		ArrayAdd(&gizmos.vertices, { Add(headStart,   radial0), color });

		ArrayAdd(&gizmos.vertices, { Add(headStart,   radial1), color });
		ArrayAdd(&gizmos.vertices, { Add(headStart,   radial0), color });
		ArrayAdd(&gizmos.vertices, { Add(from, 	      radial1), color });
	}

	// head

	// ~Todo use spheres instead?
	Vec3 points[8] = { 
		v3(-1, -1, -1),
		v3(-1, -1,  1),
		v3( 1, -1,  1),
		v3( 1, -1, -1),
		v3(-1,  1, -1),
		v3(-1,  1,  1),
		v3( 1,  1,  1),
		v3( 1,  1, -1),
	};

	int indices[36] = {
		// down
		0, 2, 1,
		0, 3, 2,

		//left
		0, 5, 4,
		0, 1, 5,

		//back
		1, 2, 6,
		1, 6, 5,

		// right
		2, 3, 7,
		2, 7, 6,

		// top
		4, 5, 6,
		4, 6, 7,

		// front
		0, 4, 7,
		0, 7, 3
	};

	for (int i = 0; i < 36; i++) {
		ArrayAdd(&gizmos.vertices, { Add(to, Mul(points[indices[i]], v3(headRadius))), color });
	}
}


void GizmoDisc(Vec3 origin, Vec3 normal, float innerRadius, float outerRadius, Vec3 color)
{
	constexpr int Disc_Segments = 32;

	Vec3 tmpBitan = v3(0, 1, 0);
	if (Dot(normal, tmpBitan) == 1)
		tmpBitan = v3(1, 0, 0);

	Vec3 tangent   = Normalize(Cross(normal, tmpBitan));
	Vec3 cotangent = Cross(tangent, normal);

	float r = 360 / Disc_Segments;
	for (int i = 0; i <= Disc_Segments; i++) // ~Todo figure out why this needed an extra segment but not the arrow?
	{
		Vec3 dir0 = Add( Mul(v3(Sin(r * i)), tangent),         Mul(v3(Cos(r * i)), cotangent) );
		Vec3 dir1 = Add( Mul(v3(Sin(r * (i + 1) )), tangent),  Mul(v3(Cos(r * (i + 1) )), cotangent) );

		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(dir0, v3(innerRadius))), color });
		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(dir0, v3(outerRadius))), color });
		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(dir1, v3(outerRadius))), color });

		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(dir0, v3(innerRadius))), color });
		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(dir1, v3(outerRadius))), color });
		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(dir1, v3(innerRadius))), color });
	}
}





void GizmoCube(Vec3 origin, Vec3 halfExtents, Vec3 color)
{
	Vec3 points[8] = { 
		v3(-1, -1, -1),
		v3(-1, -1,  1),
		v3( 1, -1,  1),
		v3( 1, -1, -1),
		v3(-1,  1, -1),
		v3(-1,  1,  1),
		v3( 1,  1,  1),
		v3( 1,  1, -1),
	};

	int indices[36] = {
		// down
		0, 2, 1,
		0, 3, 2,

		//left
		0, 5, 4,
		0, 1, 5,

		//back
		1, 2, 6,
		1, 6, 5,

		// right
		2, 3, 7,
		2, 7, 6,

		// top
		4, 5, 6,
		4, 6, 7,

		// front
		0, 4, 7,
		0, 7, 3
	};

	for (int i = 0; i < 36; i++) {
		ArrayAdd(&gizmos.vertices, { Add(origin, Mul(points[indices[i]], halfExtents)), color });
	}
}