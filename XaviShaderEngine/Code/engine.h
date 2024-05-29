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

    void ConfigureSsaoFrameBuffer(FrameBuffer& ssaoFB);

    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;
    ButtonState lastMouseRightClick = BUTTON_IDLE;

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
    GLuint frameBufferToQuadShader;
    GLuint ssaoShader;
    GLuint ssaoBlurShader;
    GLuint frameBufferToQuadShaderSSAO;
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
    std::vector<Entity> lightEntities; // iteracion rapida
    std::vector<Light> lights; // iteracion rapida

    GLuint globalPatamsOffset;
    GLuint globalPatamsSize;

    FrameBuffer defferedFrameBuffer;
    FrameBuffer ssaoFrameBuffer;
    FrameBuffer ssaoBlurFrameBuffer;

    vec3 camPos = vec3(5.0, 5.0, 5.0);

    struct Camera {
        vec3 position;
        vec3 target;
        vec3 up;
        vec3 right;
        vec3 front;
        float speed;
        float sensitivity;
        float yaw;
        float pitch;

        float fovYRad = 0.0;
        float aspRatio = 0.0;
        float zFar = 1000.0;
        float zNear = 0.1;

    };
    bool firstClick;

    Camera cam; // camera

    float iTime = 0;

    // SSAO variables
    float sampleRadius = 0.5f;
    float ssaoBias = 0.02f;
    bool rangeCheck = true;

    void KernelRotationVectors();
    GLuint ssaoNoiseTexture;

    bool displaySSAO = true;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);


// SSAO
std::vector<vec3> SamplePositionsInTangent();