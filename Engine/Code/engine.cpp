//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "ModelLoadingFunc.h"
#include "Globals.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
	GLchar  infoLogBuffer[1024] = {};
	GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
	GLsizei infoLogSize;
	GLint   success;

	char versionString[] = "#version 430\n";
	char shaderNameDefine[128];
	sprintf(shaderNameDefine, "#define %s\n", shaderName);
	char vertexShaderDefine[] = "#define VERTEX\n";
	char fragmentShaderDefine[] = "#define FRAGMENT\n";

	const GLchar* vertexShaderSource[] = {
		versionString,
		shaderNameDefine,
		vertexShaderDefine,
		programSource.str
	};
	const GLint vertexShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(vertexShaderDefine),
		(GLint)programSource.len
	};
	const GLchar* fragmentShaderSource[] = {
		versionString,
		shaderNameDefine,
		fragmentShaderDefine,
		programSource.str
	};
	const GLint fragmentShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(fragmentShaderDefine),
		(GLint)programSource.len
	};

	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vshader);
	glAttachShader(programHandle, fshader);
	glLinkProgram(programHandle);
	glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	glUseProgram(0);

	glDetachShader(programHandle, vshader);
	glDetachShader(programHandle, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
	String programSource = ReadTextFile(filepath);

	Program program = {};
	program.handle = CreateProgramFromSource(programSource, programName);
	program.filepath = filepath;
	program.programName = programName;
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

	GLint attributeCount = 0;
	glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

	for (GLuint i = 0; i < attributeCount; i++)
	{


		GLsizei bufSize = 256;
		GLsizei length = 0;
		GLint size = 0;
		GLenum type = 0;
		GLchar name[256];
		glGetActiveAttrib(program.handle, i,
			ARRAY_COUNT(name),
			&length,
			&size,
			&type,
			name);
		u8 location = glGetAttribLocation(program.handle, name);
		program.shaderLayout.attributes.push_back(VertexShaderAttribute{ location, (u8)size });

	}

	app->programs.push_back(program);

	return app->programs.size() - 1;
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex,const Program &program)
{
	GLuint returnValue = 0;
	SubMesh& subMesh = mesh.subMeshes[submeshIndex];

	for (u32 i = 0; i < (u32)subMesh.vaos.size(); ++i)
	{
		if (subMesh.vaos[i].programHandle == program.handle)
		{
			returnValue = subMesh.vaos[i].handle;////////mirarse esto si esta bine TODU
		}
	}

	if (returnValue == 0)
	{
		glGenVertexArrays(1, &returnValue);
		glBindVertexArray(returnValue)
			;

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHndle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHndle);

		auto& shaderLayout = program.shaderLayout.attributes;
		for (auto shaderIt = shaderLayout.cbegin(); shaderIt !=shaderLayout.cend() ; ++shaderIt)
		{
			bool attributeWasLinked = false;
			auto subMeshLayout = subMesh.vertexBufferLayout.attributes;
			for (auto submeshIt = subMeshLayout.cbegin(); submeshIt != subMeshLayout.cend(); ++submeshIt)
			{
				if (shaderIt->location == submeshIt->location)
				{
					const u32 index = submeshIt->location;
					const u32 ncomp = submeshIt->componentCount;
					const u32 offset = submeshIt->offset + subMesh.vertexOffset;
					const u32 stride = subMesh.vertexBufferLayout.stride;

					glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)(offset));
					glEnableVertexAttribArray(index);
					
					attributeWasLinked = true;
					break;
				}
			}
		}
		glBindVertexArray(0);

		VAO vao = { returnValue, program.handle };
		subMesh.vaos.push_back(vao);
	}
	return returnValue;
}

void Init(App* app)
{
	// TODO: Initialize your resources here!
	// - vertex buffers
	// - element/index buffers
	// - vaos
	// - programs (and retrieve uniform indices)
	// - textures

	app->openDebugInfo += "OpenGl version:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	//VBO
	glGenBuffers(1, &app->embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//EBO
	glGenBuffers(1, &app->embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//VAO
	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glEnableVertexAttribArray(0);

	//no es lo mismo que handle, no confundir esto no es lo que se envia
	app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
	const Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
	app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");
	app->diceTexIdx = LoadTexture2D(app, "dice.png");

	VertexBufferLayaout vertexBufferLayout = {};
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{0, 3, 0});
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{2, 2, 3 * sizeof(float)});
	vertexBufferLayout.stride = 5 * sizeof(float);




	app->mode = Mode_TexturedQuad;
}

void Gui(App* app)
{
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	ImGui::Text(" %s", app->openDebugInfo.c_str());
	ImGui::End();
}

void Update(App* app)
{
	// You can handle app->input keyboard/mouse here
}

void Render(App* app)
{
	switch (app->mode)
	{
	case Mode_TexturedQuad:
	{

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// 

		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		const Program& texturedMeshProgram = app->programs[app->texturedGeometryProgramIdx];
		glUseProgram(texturedMeshProgram.handle);
		
		Model& model = app->models[app->patricioModel];
		Mesh& mesh = app->meshes[model.meshIdx];

		for (u32 i = 0; i < mesh.subMeshes.size(); ++i)
		{
			GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
			glBindVertexArray(vao);

			u32 subMeshMaterialIdx = model.materialIdx[i];
			Material& submMeshMaterial = app->materials[subMeshMaterialIdx];

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, app->textures[submMeshMaterial.albedoTextureIdx].handle);
			glUniform1i(app->texturedMeshProgram_uTexture,0);

			SubMesh& submesh = mesh.subMeshes[i];
			glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);


		}


	}
	break;

	default:;
	}
}

