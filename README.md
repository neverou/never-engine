# Never Engine

Work-in-progress open world RPG game engine.

Features include:
- Deferred rendering pipeline with customisable render pass setup (Frame Graph)
- Multiple render backends
	- Vulkan
	- OpenGL (implementation incomplete, not compatible with current version of renderer)
	- Direct3D11 (implementation incomplete, not compatible with current version of renderer)
- Level and entity serialisation
- World segmentation and streaming
- Heightmap based terrain
- Editor
	- Terrain height editor
	- Terrain chunk editor
	- Entity properties
		- Embedded metadata using C preprocessor
- Windows & Linux
	- Mac WIP
- Shader custom metadata language and parser
- Custom audio pipeline
- PhysX 4.0
- Custom Immediate mode GUI framework
- Skeletal animation (but animation file loader is work in progress)
- Data-oriented design
	- Array's of structures for hot memory-paths