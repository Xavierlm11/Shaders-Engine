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
#include "Globals.h"
#include <glm/glm.hpp>

#include <random>

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

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    GLuint ReturnValue = 0;

    SubMesh& Submesh = mesh.submeshes[submeshIndex];
    for (u32 i = 0; i < (u32)Submesh.vaos.size(); ++i)
    {
        if (Submesh.vaos[i].programHandle == program.handle)
        {
            ReturnValue = Submesh.vaos[i].handle;
            break;
        }
    }

    if (ReturnValue == 0)
    {
        glGenVertexArrays(1, &ReturnValue);
        glBindVertexArray(ReturnValue);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        auto& ShaderLayout = program.shaderLayout.attributes;
        for (auto ShaderIt = ShaderLayout.cbegin(); ShaderIt != ShaderLayout.cend(); ++ShaderIt)
        {
            bool attributeWasLinked = false;
            auto SubmeshLayout = Submesh.vertexBufferLayout.attributes;
            for (auto SubmeshIt = SubmeshLayout.cbegin(); SubmeshIt != SubmeshLayout.cend(); ++SubmeshIt)
            {
                if (ShaderIt->location == SubmeshIt->location)
                {
                    const u32 index = SubmeshIt->location;
                    const u32 ncomp = SubmeshIt->componentCount;
                    const u32 offset = SubmeshIt->offset + Submesh.vertexOffset;
                    const u32 stride = Submesh.vertexBufferLayout.stride;

                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)(offset));
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }
            assert(attributeWasLinked);
        }
        glBindVertexArray(0);

        VAO vao = { ReturnValue, program.handle };
        Submesh.vaos.push_back(vao);
    }

    return ReturnValue;
}

glm::mat4 TransformScale(const vec3& scaleFactors)
{
    return glm::scale(scaleFactors);
}

glm::mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
    glm::mat4 returnVal = glm::translate(pos);
    returnVal = glm::scale(returnVal, scaleFactors);
    return returnVal;
}
void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    //Get OPENGL info.
    app->openglDebugInfo += "OpeGL version:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    //VBO
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //ebo
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
    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Water Textures
    app->ConfigureSingleFrameBuffer(app->waterReflectionDefferedFrameBuffer);
    app->ConfigureSingleFrameBuffer(app->waterRefractionDefferedFrameBuffer);
    app->ConfigureSingleFrameBuffer(app->waterFrameBuffer);

    //app->waterNormalMap = ModelLoader::LoadTexture2D(app, "normalmap.png");
    app->waterDudvMap = ModelLoader::LoadTexture2D(app, "dudvmap.png");

    app->renderToBackBuffer = LoadProgram(app, "RENDER_TO_BB.glsl", "RENDER_TO_BB");
    app->renderToFrameBuffer = LoadProgram(app, "RENDER_TO_FB.glsl", "RENDER_TO_FB");
    app->frameBufferToQuadShader = LoadProgram(app, "FB_TO_BB.glsl", "FB_TO_BB");
    app->ssaoShader = LoadProgram(app, "SSAO.glsl", "SSAO");
    app->ssaoBlurShader = LoadProgram(app, "Blur.glsl", "Blur");
    app->frameBufferToQuadShaderSSAO = LoadProgram(app, "FB_TO_BB_SSAO.glsl", "FB_TO_BB_SSAO");
    app->waterShader = LoadProgram(app, "WaterEffect.glsl", "WaterEffect");

    const Program& texturedMeshProgram = app->programs[app->renderToBackBuffer];
    app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
    //u32 PatrickModelindex = ModelLoader::LoadModel(app, "Patrick/Patrick.obj");
    //u32 SpongeModelindex = ModelLoader::LoadModel(app, "Patrick/SpongeBob.obj");
    //u32 GroundModelindex = ModelLoader::LoadModel(app, "Patrick/ground.obj");
    //u32 HouseModelindex = ModelLoader::LoadModel(app, "Assets/hut.obj");
    //u32 BookShelfindex = ModelLoader::LoadModel(app, "Assets/light_oak_bookshelf.obj");
    u32 houseShelfindex = ModelLoader::LoadModel(app, "Assets/world.obj");
    u32 lakeindex = ModelLoader::LoadModel(app, "Assets/Lake.obj");
    u32 waterindex = ModelLoader::LoadModel(app, "Assets/WaterPlane.obj");

    u32 SphereModelindex = ModelLoader::LoadModel(app, "Patrick/Sphere.obj");
    u32 ConeModelindex = ModelLoader::LoadModel(app, "Patrick/Cone.obj");

    glEnable(GL_DEPTH_TEST);////////////////////////////////////////////////////////////////// PARA PROFUNDIIDAD
    glEnable(GL_CULL_FACE); // para que no pinte normales si no se ven

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->localUniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize); // crea un uniform buffer

    //app->entities.push_back({ TransformPositionScale(vec3(5.0,0.0,-3.0),vec3(1.0,1.0,1.0)), PatrickModelindex,0,0 });
    //app->entities.push_back({ TransformPositionScale(vec3(-5.0,0.0,-3.0),vec3(1.0,1.0,1.0)), PatrickModelindex,0,0 });
    //app->entities.push_back({ TransformPositionScale(vec3(5.0,-2.0,-2.0),vec3(1.0,1.0,1.0)), PatrickModelindex,0,0 });
    //app->entities.push_back({ TransformPositionScale(vec3(5.0,-2.0,-2.0),vec3(1.0,1.0,1.0)), SpongeModelindex,0,0 });
    //app->entities.push_back({ TransformPositionScale(vec3(0.0,-2.0,3.0),vec3(1.0,1.0,1.0)), SpongeModelindex,0,0 });

    //app->entities.push_back({ TransformPositionScale(vec3(0.0, -5.0, 0.0), vec3(1.0, 1.0, 1.0)), GroundModelindex,0,0 });
    //app->entities.push_back({ TransformPositionScale(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), HouseModelindex,0,0 });
    //app->entities.push_back({ TransformPositionScale(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), BookShelfindex,0,0 });
    app->entities.push_back({ TransformPositionScale(vec3(0.0, 2.0, 0.0), vec3(1.0, 1.0, 1.0)), houseShelfindex,0,0 });
    app->entities.push_back({ TransformPositionScale(vec3(-45.0, 0.0, 65.0), vec3(1.0, 1.0, 1.0)), lakeindex,0,0 });


    app->lights.push_back({ LightType::LightType_Directional,vec3(1.0,1.0,1.0),vec3(1.0,1.0,1.0),vec3(0.0,5.0,0.0) });
    app->lights.push_back({ LightType::LightType_Point,vec3(0.0,0.0,3.0),vec3(1.0,1.0,1.0),vec3(10.0,2.0,1.0) });
    app->lights.push_back({ LightType::LightType_Point,vec3(3.0,0.0,0.0),vec3(1.0,1.0,1.0),vec3(-10.0,2.0,1.0) });

    for (int i = 0; i < app->lights.size(); i++)
    {
        if (app->lights[i].type == LightType::LightType_Point)
            app->entities.push_back({ TransformPositionScale((app->lights[i].position), vec3(0.5, 0.5, 0.5)), SphereModelindex,0,0 });
        else
            app->entities.push_back({ TransformPositionScale((app->lights[i].position), (-app->lights[i].direction)), ConeModelindex,0,0 });

    }

    app->ConfigureFrameBuffer(app->defferedFrameBuffer);
    app->ConfigureFrameBuffer(app->waterRefractionFrameBuffer);
    app->ConfigureFrameBuffer(app->waterReflectionFrameBuffer);
    app->ConfigureSingleFrameBuffer(app->ssaoFrameBuffer);
    app->ConfigureSingleFrameBuffer(app->ssaoBlurFrameBuffer);

    app->cam.position = vec3(9.0f, 2.0f, 15.0f);
    app->cam.target = vec3(0.0f, 0.0f, -1.0f);
    app->cam.up = vec3(0.0f, 1.0f, 0.0f);
    app->cam.speed = 1.0f;
    app->cam.sensitivity = 0.05f;
    app->cam.front = vec3(0.0f, 0.0f, -1.0f);
    app->cam.pitch = 0;
    app->cam.yaw = -90;
    app->firstClick = true;

    app->mode = Mode_Deferred;

    app->KernelRotationVectors();
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    ImGui::Text("%s", app->openglDebugInfo.c_str());

    const char* renderModes[] = { "FORWARD","DEFERRED" };
    if (ImGui::BeginCombo("Render Mode", renderModes[app->mode]))
    {
        for (size_t i = 0; i < ARRAY_COUNT(renderModes); ++i)
        {
            bool isSelected = (i == app->mode);
            if (ImGui::Selectable(renderModes[i], isSelected)) //recordar esto no va bien
            {
                app->mode = static_cast<Mode>(i);
            }
        }

        ImGui::EndCombo();
    }
    if (app->mode == Mode::Mode_Deferred)
    {
        const char* renderBuffers[] = { "MAIN","REFRACTION","REFLECTION"};
        if (ImGui::BeginCombo("Render Buffer", renderBuffers[app->renderBuffers]))
        {
            for (size_t i = 0; i < ARRAY_COUNT(renderBuffers); ++i)
            {
                bool isSelected = (i == app->renderBuffers);
                if (ImGui::Selectable(renderBuffers[i], isSelected)) //recordar esto no va bien
                {
                    app->renderBuffers = static_cast<Mode>(i);
                }
            }

            ImGui::EndCombo();
        }
        if (app->renderBuffers == 0)
        {
            ImGui::Checkbox("Use SSAO", &app->displaySSAO);
            ImGui::SliderFloat("Sample Radius", &app->sampleRadius, 0.0f, 100.0f);
            ImGui::SliderFloat("SSAO Bias", &app->ssaoBias, 0.0f, 100.0f);
            const char* modes[] = { "Albedo", "Normals", "Position", "ViewDir", "Depth" };
            for (size_t i = 0; i < app->defferedFrameBuffer.colorAttachment.size(); i++)
            {
                ImGui::Text(modes[i]);
                ImGui::Image((ImTextureID)app->defferedFrameBuffer.colorAttachment[i], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
            }
            ImGui::Text("Ambient Occlusion");
            ImGui::Image((ImTextureID)app->ssaoFrameBuffer.colorAttachment[0], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::Text("Ambient Occlusion with Blur");
            ImGui::Image((ImTextureID)app->ssaoBlurFrameBuffer.colorAttachment[0], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::Text("WaterTexture");
            ImGui::Image((ImTextureID)app->waterFrameBuffer.colorAttachment[0], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
        }
        else if (app->renderBuffers == 1)
        {
            const char* modes[] = { "Albedo", "Normals", "Position", "ViewDir", "Depth" };
            for (size_t i = 0; i < app->waterRefractionFrameBuffer.colorAttachment.size(); i++)
            {
                ImGui::Text(modes[i]);
                ImGui::Image((ImTextureID)app->waterRefractionFrameBuffer.colorAttachment[i], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
            }
            ImGui::Text("Deffered Refraction");
            ImGui::Image((ImTextureID)app->waterRefractionDefferedFrameBuffer.colorAttachment[0], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            const char* modes[] = { "Albedo", "Normals", "Position", "ViewDir", "Depth" };
            for (size_t i = 0; i < app->waterReflectionFrameBuffer.colorAttachment.size(); i++)
            {
                ImGui::Text(modes[i]);
                ImGui::Image((ImTextureID)app->waterReflectionFrameBuffer.colorAttachment[i], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
            }
            ImGui::Text("Deffered Reflection");
            ImGui::Image((ImTextureID)app->waterReflectionDefferedFrameBuffer.colorAttachment[0], ImVec2(300, 150), ImVec2(0, 1), ImVec2(1, 0));
        }
    }
    
    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    bool movingCam = false;
    float sensivity = 1.4f;

    if (app->input.keys[Key::K_W] == ButtonState::BUTTON_PRESSED)
    {
        app->cam.position += app->cam.front * app->cam.speed;
    }
    if (app->input.keys[Key::K_A] == ButtonState::BUTTON_PRESSED)
    {
        app->cam.position -= glm::normalize(cross(app->cam.front, app->cam.up)) * app->cam.speed;
    }
    if (app->input.keys[Key::K_S] == ButtonState::BUTTON_PRESSED)
    {
        app->cam.position -= app->cam.front * app->cam.speed;
    }
    if (app->input.keys[Key::K_D] == ButtonState::BUTTON_PRESSED)
    {
        app->cam.position += glm::normalize(cross(app->cam.front, app->cam.up)) * app->cam.speed;
    }
    if (app->input.keys[Key::K_Q] == ButtonState::BUTTON_PRESSED)
    {
        app->cam.position -= app->cam.up * app->cam.speed;
    }
    if (app->input.keys[Key::K_E] == ButtonState::BUTTON_PRESSED)
    {
        app->cam.position += app->cam.up * app->cam.speed;
    }

    // Actualizar dirección frontal (front)
    app->cam.right = normalize(cross(glm::vec3(0.0f, 1.0f, 0.0f), app->cam.front));
    app->cam.up = normalize(cross(app->cam.front, app->cam.right));
    app->cam.target = app->cam.position + app->cam.front;

    app->camInv = app->cam;
    app->camInv.position.y *= -1;
    app->camInv.pitch *= -1;
    if (app->camInv.pitch > 89.0f) app->camInv.pitch = 89.0f;
    if (app->camInv.pitch < -89.0f) app->camInv.pitch = -89.0f;
    vec3 front;
    front.x = cos(glm::radians(app->camInv.yaw)) * cos(glm::radians(app->camInv.pitch));
    front.y = sin(glm::radians(app->camInv.pitch));
    front.z = sin(glm::radians(app->camInv.yaw)) * cos(glm::radians(app->camInv.pitch));
    app->camInv.front = glm::normalize(front);

    app->camInv.right = normalize(cross(glm::vec3(0.0f, 1.0f, 0.0f), app->camInv.front));
    app->camInv.up = normalize(cross(app->camInv.front, app->camInv.right));
    app->camInv.target = app->camInv.position + app->camInv.front;
}


void Render(App* app)
{
    switch (app->mode)
    {
    case Mode_Forward:
    {
        app->UpdateEntityBuffer(&app->cam);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, app->displaySize.x, app->displaySize.y);
        //glBindFramebuffer(GL_FRAMEBUFFER, app->defferedFrameBuffer.fbHandle);

        const Program& ForwardProgram = app->programs[app->renderToBackBuffer];
        glUseProgram(ForwardProgram.handle);

        app->RenderGeometry(ForwardProgram, vec4(0.0f));

        //BufferManager::UnindBuffer(app->localUniformBuffer);
    }
    break;
    case Mode_Deferred:
    {
        // Refraction Pass
        glEnable(GL_CLIP_DISTANCE0);
        app->UpdateEntityBuffer(&app->cam);

        //RENDER TO FB COLORaTT
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);
        glBindFramebuffer(GL_FRAMEBUFFER, app->waterRefractionFrameBuffer.fbHandle);
        glDrawBuffers(app->waterRefractionFrameBuffer.colorAttachment.size(), app->waterRefractionFrameBuffer.colorAttachment.data());
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& DeferredProgram = app->programs[app->renderToFrameBuffer];
        glUseProgram(DeferredProgram.handle);

        app->RenderGeometry(DeferredProgram, vec4(0, 1, 0, 0));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        app->WaterPass(&app->cam, GL_COLOR_ATTACHMENT0, false);

        // Reflection Pass
        app->UpdateEntityBuffer(&app->camInv);

        //RENDER TO FB COLORaTT
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);
        glBindFramebuffer(GL_FRAMEBUFFER, app->waterReflectionFrameBuffer.fbHandle);
        glDrawBuffers(app->waterReflectionFrameBuffer.colorAttachment.size(), app->waterReflectionFrameBuffer.colorAttachment.data());
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(DeferredProgram.handle);

        app->RenderGeometry(DeferredProgram, vec4(0, -1, 0, 0));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        app->WaterPass(&app->camInv, GL_COLOR_ATTACHMENT0, true);

        glDisable(GL_CLIP_DISTANCE0);

        // Main Pass
        app->UpdateEntityBuffer(&app->cam);

        //RENDER TO FB COLORaTT
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);
        glBindFramebuffer(GL_FRAMEBUFFER, app->defferedFrameBuffer.fbHandle);
        glDrawBuffers(app->defferedFrameBuffer.colorAttachment.size(), app->defferedFrameBuffer.colorAttachment.data());
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(DeferredProgram.handle);

        app->RenderGeometry(DeferredProgram, vec4(0.0f));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        //RENDER SSAO
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        glBindFramebuffer(GL_FRAMEBUFFER, app->ssaoFrameBuffer.fbHandle);
        glDrawBuffers(app->ssaoFrameBuffer.colorAttachment.size(), app->ssaoFrameBuffer.colorAttachment.data());
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& SsaoProgram = app->programs[app->ssaoShader];
        glUseProgram(SsaoProgram.handle);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[1]);
        glUniform1i(glGetUniformLocation(SsaoProgram.handle, "uNormals"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[2]);
        glUniform1i(glGetUniformLocation(SsaoProgram.handle, "uPosition"), 1);

        auto firstSamplePoint = SamplePositionsInTangent();
        glUniform3fv(glGetUniformLocation(SsaoProgram.handle, "ssaoSamples"), 64, &firstSamplePoint.data()->x);

        glUniform1f(glGetUniformLocation(SsaoProgram.handle, "sampleRadius"), app->sampleRadius);

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), app->cam.aspRatio, app->cam.zNear, app->cam.zFar);
        glUniformMatrix4fv(glGetUniformLocation(SsaoProgram.handle, "projectionMatrix"), 1, GL_FALSE, &projection[0][0]);

        glUniform2f(glGetUniformLocation(SsaoProgram.handle, "viewportSize"), app->displaySize.x, app->displaySize.y);

        glUniform1f(glGetUniformLocation(SsaoProgram.handle, "ssaoBias"), app->ssaoBias);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->ssaoNoiseTexture);
        glUniform1i(glGetUniformLocation(SsaoProgram.handle, "noiseTexture"), 2);

        glBindVertexArray(app->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //RENDER SSAO Blur
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        glBindFramebuffer(GL_FRAMEBUFFER, app->ssaoBlurFrameBuffer.fbHandle);
        glDrawBuffers(app->ssaoBlurFrameBuffer.colorAttachment.size(), app->ssaoBlurFrameBuffer.colorAttachment.data());
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& SsaoBlurProgram = app->programs[app->ssaoBlurShader];
        glUseProgram(SsaoBlurProgram.handle);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->ssaoFrameBuffer.colorAttachment[0]);
        glUniform1i(glGetUniformLocation(SsaoBlurProgram.handle, "ssaoTexture"), 0);

        glBindVertexArray(app->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Render Water
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        glBindFramebuffer(GL_FRAMEBUFFER, app->waterFrameBuffer.fbHandle);
        glDrawBuffers(app->waterFrameBuffer.colorAttachment.size(), app->waterFrameBuffer.colorAttachment.data());
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& waterProgram = app->programs[app->waterShader];
        glUseProgram(waterProgram.handle);

        vec3 xCam = glm::cross(app->cam.front, vec3(0, 1, 0));
        vec3 yCam = glm::cross(xCam, app->cam.front);
        glm::mat4 view = glm::lookAt(app->cam.position, app->cam.target, yCam);
        glUniformMatrix4fv(glGetUniformLocation(waterProgram.handle, "viewMatrix"), 1, GL_FALSE, &view[0][0]);

        glUniformMatrix4fv(glGetUniformLocation(waterProgram.handle, "projectionMatrix"), 1, GL_FALSE, &projection[0][0]);

        glUniform2f(glGetUniformLocation(waterProgram.handle, "viewportSize"), app->displaySize.x, app->displaySize.y);

        glm::mat4 viewInv = glm::inverse(view);
        glUniformMatrix4fv(glGetUniformLocation(waterProgram.handle, "viewMatrixInv"), 1, GL_FALSE, &viewInv[0][0]);

        glm::mat4 projectionInv = glm::inverse(projection);
        glUniformMatrix4fv(glGetUniformLocation(waterProgram.handle, "projectionMatrixInv"), 1, GL_FALSE, &projectionInv[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->waterReflectionDefferedFrameBuffer.colorAttachment[0]);
        glUniform1i(glGetUniformLocation(waterProgram.handle, "reflectionMap"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->waterRefractionDefferedFrameBuffer.colorAttachment[0]);
        glUniform1i(glGetUniformLocation(waterProgram.handle, "refractionMap"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[4]);
        glUniform1i(glGetUniformLocation(waterProgram.handle, "refractionDepth"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, app->textures[app->waterDudvMap].handle);
        glUniform1i(glGetUniformLocation(waterProgram.handle, "dudvMap"), 3);

        glBindVertexArray(app->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        //RENDER TO bb FROM cOLORaTT
        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        if (!app->displaySSAO)
        {
            const Program& FBToBB = app->programs[app->frameBufferToQuadShader];
            glUseProgram(FBToBB.handle);

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->localUniformBuffer.handle, app->globalPatamsOffset, app->globalPatamsSize);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[0]);
            glUniform1i(glGetUniformLocation(FBToBB.handle, "uAlbedo"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[1]);
            glUniform1i(glGetUniformLocation(FBToBB.handle, "uNormals"), 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[2]);
            glUniform1i(glGetUniformLocation(FBToBB.handle, "uPosition"), 2);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[3]);
            glUniform1i(glGetUniformLocation(FBToBB.handle, "uViewDir"), 3);

            glBindVertexArray(app->vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
        }
        else
        {
            const Program& FBToBBwithSSAO = app->programs[app->frameBufferToQuadShaderSSAO];
            glUseProgram(FBToBBwithSSAO.handle);

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->localUniformBuffer.handle, app->globalPatamsOffset, app->globalPatamsSize);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[0]);
            glUniform1i(glGetUniformLocation(FBToBBwithSSAO.handle, "uAlbedo"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[1]);
            glUniform1i(glGetUniformLocation(FBToBBwithSSAO.handle, "uNormals"), 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[2]);
            glUniform1i(glGetUniformLocation(FBToBBwithSSAO.handle, "uPosition"), 2);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[3]);
            glUniform1i(glGetUniformLocation(FBToBBwithSSAO.handle, "uViewDir"), 3);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, app->ssaoFrameBuffer.colorAttachment[0]);
            glUniform1i(glGetUniformLocation(FBToBBwithSSAO.handle, "uAO"), 4);

            glBindVertexArray(app->vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
        }
        glUseProgram(0);
    }
    break;
    default:;
    }
}

void App::UpdateEntityBuffer(Camera* camera)
{
    camera->aspRatio = (float)displaySize.x / (float)displaySize.y;
    camera->fovYRad = glm::radians(60.0f);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), camera->aspRatio, camera->zNear, camera->zFar);

    vec3 xCam = glm::cross(camera->front, vec3(0, 1, 0));
    vec3 yCam = glm::cross(xCam, camera->front);

    glm::mat4 view = glm::lookAt(camera->position, camera->target, yCam);
    BufferManager::MapBuffer(localUniformBuffer, GL_WRITE_ONLY);

    //push lights globals paramas
    globalPatamsOffset = localUniformBuffer.head;
    PushVec3(localUniformBuffer, camera->position);
    PushUInt(localUniformBuffer, lights.size());
    for (u32 i = 0; i < lights.size(); ++i)
    {
        BufferManager::AlignHead(localUniformBuffer, sizeof(vec4));

        Light& light = lights[i];
        PushUInt(localUniformBuffer, light.type);
        PushVec3(localUniformBuffer, light.color);
        PushVec3(localUniformBuffer, light.direction);
        PushVec3(localUniformBuffer, light.position);
    }

    globalPatamsSize = localUniformBuffer.head - globalPatamsOffset;

    //local parms
    u32 iteration = 0;
    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        glm::mat4 world = it->worldMatrix;
        glm::mat4 WVP = projection * view * world; //wordl view projection

        Buffer& localBuffer = localUniformBuffer;
        BufferManager::AlignHead(localBuffer, uniformBlockAlignment);
        it->localParamsOffset = localBuffer.head;
        PushMat4(localBuffer, world);
        PushMat4(localBuffer, WVP);
        it->localParamsSize = localBuffer.head - it->localParamsOffset;
        ++iteration;
    }
    BufferManager::UnmapBuffer(localUniformBuffer);
}

void App::ConfigureFrameBuffer(FrameBuffer& aConfigFb)
{
    //GLuint NUMBER_OF_CA = 3; // esto cambeir si se añaden en el shader


    aConfigFb.colorAttachment.push_back(CreateTexture());
    aConfigFb.colorAttachment.push_back(CreateTexture(true));
    aConfigFb.colorAttachment.push_back(CreateTexture(true));
    aConfigFb.colorAttachment.push_back(CreateTexture(true));
    aConfigFb.colorAttachment.push_back(CreateTexture(true));
    //aConfigFb.colorAttachment.push_back(CreateTexture());

    glGenTextures(1, &aConfigFb.depthHandle);
    glBindTexture(GL_TEXTURE_2D, aConfigFb.depthHandle);
    //EL BUFFEER OCUPA MAS,, si hay o¡problema scon el z fight aumentar la cantidad de bits
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displaySize.x, displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);  //RGBA para .si hacemos un resice que vuelva a generar el frame buff
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &aConfigFb.fbHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, aConfigFb.fbHandle);

    std::vector<GLuint> drawBuffers;
    for (size_t i = 0; i < aConfigFb.colorAttachment.size(); ++i)
    {
        GLuint position = GL_COLOR_ATTACHMENT0 + i;
        glFramebufferTexture(GL_FRAMEBUFFER, position, aConfigFb.colorAttachment[i], 0);
        drawBuffers.push_back(position);
    }

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, aConfigFb.depthHandle, 0);

    glDrawBuffers(drawBuffers.size(), drawBuffers.data());


    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        int i = 0;
    }

    //GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    /// Aqui va u nif , TUDU
// glDrawBuffers(1, &app->colorAttachmentHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void App::ConfigureSingleFrameBuffer(FrameBuffer& ssaoFB)
{
    ssaoFB.colorAttachment.push_back(CreateTexture());

    glGenTextures(1, &ssaoFB.depthHandle);
    glBindTexture(GL_TEXTURE_2D, ssaoFB.depthHandle);
    //EL BUFFEER OCUPA MAS,, si hay o¡problema scon el z fight aumentar la cantidad de bits
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displaySize.x, displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);  //RGBA para .si hacemos un resice que vuelva a generar el frame buff
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &ssaoFB.fbHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFB.fbHandle);

    std::vector<GLuint> drawBuffers;
    for (size_t i = 0; i < ssaoFB.colorAttachment.size(); ++i)
    {
        GLuint position = GL_COLOR_ATTACHMENT0 + i;
        glFramebufferTexture(GL_FRAMEBUFFER, position, ssaoFB.colorAttachment[i], 0);
        drawBuffers.push_back(position);
    }

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ssaoFB.depthHandle, 0);

    glDrawBuffers(drawBuffers.size(), drawBuffers.data());

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        int i = 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void App::RenderGeometry(const Program& aBindedProgram, vec4 clippingPlane)
{
    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), localUniformBuffer.handle, globalPatamsOffset, globalPatamsSize);
    for (auto it = entities.begin(); it != entities.end(); ++it)
    {

        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), localUniformBuffer.handle, it->localParamsOffset, it->localParamsSize); //todu creo q aqui no va, creo que es modelloading que no va

        Model& model = models[it->modelIndex];
        Mesh& mesh = meshes[model.meshIdx];


        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, aBindedProgram);
            glBindVertexArray(vao);

            u32 subMeshmaterialIdx = model.materialIdx[i];
            Material subMeshMaterial = materials[subMeshmaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.albedoTextureIdx].handle);
            glUniform1i(texturedMeshProgram_uTexture, 0);

            glUniform4f(glGetUniformLocation(aBindedProgram.handle, "clippingPlane"), clippingPlane.x, clippingPlane.y, clippingPlane.z, clippingPlane.w);

            vec3 xCam = glm::cross(cam.front, vec3(0, 1, 0));
            vec3 yCam = glm::cross(xCam, cam.front);
            glm::mat4 view = glm::lookAt(cam.position, cam.target, yCam);
            glUniformMatrix4fv(glGetUniformLocation(aBindedProgram.handle, "viewMatrix"), 1, GL_FALSE, &view[0][0]);

            SubMesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
        }

    }

}

const GLuint App::CreateTexture(const bool isFloatingPoint)
{
    GLuint textureHandle;

    GLenum internalFormat = isFloatingPoint ? GL_RGBA16F : GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum dataType = isFloatingPoint ? GL_FLOAT : GL_UNSIGNED_BYTE;

    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, displaySize.x, displaySize.y, 0, format, dataType, NULL);  //RGBA para .si hacemos un resice que vuelva a generar el frame buff
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureHandle;
}

float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

std::vector<vec3> SamplePositionsInTangent()
{
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        float scale = (float)i / 64.0;
        scale = Lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;

        ssaoKernel.push_back(sample);
    }
    return ssaoKernel;
}

void App::KernelRotationVectors()
{
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &ssaoNoiseTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void App::WaterPass(Camera* camera, GLenum ca, bool isReflectionPart)
{
    glClearColor(0.f, 0.f, 0.f, .0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, displaySize.x, displaySize.y);

    if (isReflectionPart) glBindFramebuffer(GL_FRAMEBUFFER, waterReflectionDefferedFrameBuffer.fbHandle);
    else glBindFramebuffer(GL_FRAMEBUFFER, waterRefractionDefferedFrameBuffer.fbHandle);

    glDrawBuffer(ca);
    glClearColor(0.f, 0.f, 0.f, .0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const Program& program = programs[frameBufferToQuadShader];
    glUseProgram(program.handle);

    if (isReflectionPart)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waterReflectionFrameBuffer.colorAttachment[0]);
        glUniform1i(glGetUniformLocation(program.handle, "uAlbedo"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, waterReflectionFrameBuffer.colorAttachment[1]);
        glUniform1i(glGetUniformLocation(program.handle, "uNormals"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, waterReflectionFrameBuffer.colorAttachment[2]);
        glUniform1i(glGetUniformLocation(program.handle, "uPosition"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, waterReflectionFrameBuffer.colorAttachment[3]);
        glUniform1i(glGetUniformLocation(program.handle, "uViewDir"), 3);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waterRefractionFrameBuffer.colorAttachment[0]);
        glUniform1i(glGetUniformLocation(program.handle, "uAlbedo"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, waterRefractionFrameBuffer.colorAttachment[1]);
        glUniform1i(glGetUniformLocation(program.handle, "uNormals"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, waterRefractionFrameBuffer.colorAttachment[2]);
        glUniform1i(glGetUniformLocation(program.handle, "uPosition"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, waterRefractionFrameBuffer.colorAttachment[3]);
        glUniform1i(glGetUniformLocation(program.handle, "uViewDir"), 3);
    }

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}