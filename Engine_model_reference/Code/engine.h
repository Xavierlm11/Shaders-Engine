//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "BufferSupFuncs.h"
#include "ModelLoadingFuncs.h"
#include "Globals.h"

const VertexV3V2 vertices[] = {
	{glm::vec3(-1.0,-1.0,0.0), glm::vec2(0.0,0.0)},
	{glm::vec3(1.0,-1.0,0.0), glm::vec2(1.0,0.0)},
	{glm::vec3(1.0,1.0,0.0), glm::vec2(1.0,1.0)},
	{glm::vec3(-1.0,1.0,0.0), glm::vec2(0.0,1.0)},
};

const u16 indices[] =
{
	0,1,2,
	0,2,3
};

struct App
{
	void UpdateEntityBuffer();

	void ConfigureFrameBuffer(FrameBuffer& aConfigFb);

	void RenderGeometry(const Program& aBindedProgram);

	const GLuint CreateTexture(const bool isFloatingPoint = false);

	// Loop
	f32  deltaTime;
	bool isRunning;

	// Input
	Input input;

	// Graphics
	char gpuName[64];
	char openGlVersion[64];

	ivec2 displaySize;

	std::vector<Texture>    textures;
	std::vector<Material>   materials;
	std::vector<Mesh>       meshes;
	std::vector<Model>      models;
	std::vector<Program>    programs;

	// program indices
	GLuint renderToBackBuffer;
	GLuint renderToFrameBuffer;
	GLuint FrameBufferToQuadShader;

	u32 patricioModel = 0;
	GLuint texturedMeshProgram_uTexture;

	// texture indices
	u32 diceTexIdx;
	u32 whiteTexIdx;
	u32 blackTexIdx;
	u32 normalTexIdx;
	u32 magentaTexIdx;

	// Mode
	Mode mode;

	// Embedded geometry (in-editor simple meshes such as
	// a screen filling quad, a cube, a sphere...)
	GLuint embeddedVertices;
	GLuint embeddedElements;

	// Location of the texture uniform in the textured quad shader
	GLuint programUniformTexture;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint vao;

	std::string openglDebugInfo;


	GLint maxUniformBufferSize;
	GLint uniformBlockAlignment;//alignemnt entre uniform block no entre las variables
	Buffer localUniformBuffer;//donde estan todos las variables de los patricios
	std::vector<Entity> entities; // iteracion rapida
	std::vector<Light> lights; // iteracion rapida

	GLuint globalPatamsOffset;
	GLuint globalPatamsSize;

	FrameBuffer defferedFrameBuffer;

	vec3 camPos = vec3(5.0, 5.0, 5.0);

	float iTime=0;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

