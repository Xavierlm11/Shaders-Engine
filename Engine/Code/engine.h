//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "BufferFunc.h"

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Count
};

struct VertexV3V2 {
    glm::vec3 pos;
    glm::vec2 uv;
};

const VertexV3V2 vertices[] = {
    {glm::vec3(-0.5,-0.5,0.0),glm::vec2(0.0,0.0)},
    {glm::vec3(0.5,-0.5,0.0),glm::vec2(1.0,0.0)},
    {glm::vec3(0.5,0.5,0.0),glm::vec2(1.0,1.0)},
    {glm::vec3(-0.5,0.5,0.0),glm::vec2(0.0,1.0)},
};

const u16 indices[] =
{
    0,1,2,
    0,2,3
};

struct App
{

    App() : deltaTime(0.0f), isRunning(true)
    {

    }
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Program>  programs;

    // program indices
    u32 texturedGeometryProgramIdx;
    
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

    std::string openDebugInfo;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

