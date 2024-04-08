#include "ModelLoadingFunc.h"

#include "engine.h"

#include <stb_image.h>
#include <stb_image_write.h>
namespace ModelLoader
{


    Image LoadImage(const char* filename)
    {
        Image img = {};
        stbi_set_flip_vertically_on_load(true);
        img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
        if (img.pixels)
        {
            img.stride = img.size.x * img.nchannels;
        }
        else
        {
            ELOG("Could not open file %s", filename);
        }
        return img;
    }

    void FreeImage(Image image)
    {
        stbi_image_free(image.pixels);
    }

    GLuint CreateTexture2DFromImage(Image image)
    {
        GLenum internalFormat = GL_RGB8;
        GLenum dataFormat = GL_RGB;
        GLenum dataType = GL_UNSIGNED_BYTE;

        switch (image.nchannels)
        {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
        }

        GLuint texHandle;
        glGenTextures(1, &texHandle);
        glBindTexture(GL_TEXTURE_2D, texHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        return texHandle;
    }

    u32 LoadTexture2D(App* app, const char* filepath)
    {
        for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
            if (app->textures[texIdx].filepath == filepath)
                return texIdx;

        Image image = LoadImage(filepath);

        if (image.pixels)
        {
            Texture tex = {};
            tex.handle = CreateTexture2DFromImage(image);
            tex.filepath = filepath;

            u32 texIdx = app->textures.size();
            app->textures.push_back(tex);

            FreeImage(image);
            return texIdx;
        }
        else
        {
            return UINT32_MAX;
        }
    }
    void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
    {
        std::vector<float> vertices;
        std::vector<u32> indices;

        bool hasTexCoords = false;
        bool hasTangentSpace = false;

        // process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            vertices.push_back(mesh->mVertices[i].x);
            vertices.push_back(mesh->mVertices[i].y);
            vertices.push_back(mesh->mVertices[i].z);
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);

            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                hasTexCoords = true;
                vertices.push_back(mesh->mTextureCoords[0][i].x);
                vertices.push_back(mesh->mTextureCoords[0][i].y);
            }

            if (mesh->mTangents != nullptr && mesh->mBitangents)
            {
                hasTangentSpace = true;
                vertices.push_back(mesh->mTangents[i].x);
                vertices.push_back(mesh->mTangents[i].y);
                vertices.push_back(mesh->mTangents[i].z);

                // For some reason ASSIMP gives me the bitangents flipped.
                // Maybe it's my fault, but when I generate my own geometry
                // in other files (see the generation of standard assets)
                // and all the bitangents have the orientation I expect,
                // everything works ok.
                // I think that (even if the documentation says the opposite)
                // it returns a left-handed tangent space matrix.
                // SOLUTION: I invert the components of the bitangent here.
                vertices.push_back(-mesh->mBitangents[i].x);
                vertices.push_back(-mesh->mBitangents[i].y);
                vertices.push_back(-mesh->mBitangents[i].z);
            }
        }

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        // store the proper (previously proceessed) material for this mesh
        submeshMaterialIndices.push_back(baseMeshMaterialIndex + mesh->mMaterialIndex);

        // create the vertex format
        VertexBufferLayaout vertexBufferLayout = {};
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
        vertexBufferLayout.stride = 6 * sizeof(float);
        if (hasTexCoords)
        {
            vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
            vertexBufferLayout.stride += 2 * sizeof(float);
        }
        if (hasTangentSpace)
        {
            vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexBufferLayout.stride });
            vertexBufferLayout.stride += 3 * sizeof(float);

            vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexBufferLayout.stride });
            vertexBufferLayout.stride += 3 * sizeof(float);
        }

        // add the submesh into the mesh
        SubMesh submesh = {};
        submesh.vertexBufferLayout = vertexBufferLayout;
        submesh.vertices.swap(vertices);
        submesh.indices.swap(indices);
        myMesh->subMeshes.push_back(submesh);
    }

    void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory)
    {
        aiString name;
        aiColor3D diffuseColor;
        aiColor3D emissiveColor;
        aiColor3D specularColor;
        ai_real shininess;
        material->Get(AI_MATKEY_NAME, name);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
        material->Get(AI_MATKEY_SHININESS, shininess);

        myMaterial.name = name.C_Str();
        myMaterial.albedo = vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        myMaterial.emissive = vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
        myMaterial.smoothness = shininess / 256.0f;

        aiString aiFilename;
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            material->GetTexture(aiTextureType_DIFFUSE, 0, &aiFilename);
            String filename = MakeString(aiFilename.C_Str());
            String filepath = MakePath(directory, filename);
            myMaterial.albedoTextureIdx = LoadTexture2D(app, filepath.str);
        }
        if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
        {
            material->GetTexture(aiTextureType_EMISSIVE, 0, &aiFilename);
            String filename = MakeString(aiFilename.C_Str());
            String filepath = MakePath(directory, filename);
            myMaterial.emissiveTextureIdx = LoadTexture2D(app, filepath.str);
        }
        if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
        {
            material->GetTexture(aiTextureType_SPECULAR, 0, &aiFilename);
            String filename = MakeString(aiFilename.C_Str());
            String filepath = MakePath(directory, filename);
            myMaterial.specularTextureIdx = LoadTexture2D(app, filepath.str);
        }
        if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            material->GetTexture(aiTextureType_NORMALS, 0, &aiFilename);
            String filename = MakeString(aiFilename.C_Str());
            String filepath = MakePath(directory, filename);
            myMaterial.normalsTextureIdx = LoadTexture2D(app, filepath.str);
        }
        if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
        {
            material->GetTexture(aiTextureType_HEIGHT, 0, &aiFilename);
            String filename = MakeString(aiFilename.C_Str());
            String filepath = MakePath(directory, filename);
            myMaterial.bumpTextureIdx = LoadTexture2D(app, filepath.str);
        }

        //myMaterial.createNormalFromBump();
    }

    void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
    {
        // process all the node's meshes (if any)
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessAssimpMesh(scene, mesh, myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
        }

        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessAssimpNode(scene, node->mChildren[i], myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
        }
    }

    u32 LoadModel(App* app, const char* filename)
    {
        const aiScene* scene = aiImportFile(filename,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_PreTransformVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            aiProcess_SortByPType);

        if (!scene)
        {
            ELOG("Error loading mesh %s: %s", filename, aiGetErrorString());
            return UINT32_MAX;
        }

        app->meshes.push_back(Mesh{});
        Mesh& mesh = app->meshes.back();
        u32 meshIdx = (u32)app->meshes.size() - 1u;

        app->models.push_back(Model{});
        Model& model = app->models.back();
        model.meshIdx = meshIdx;
        u32 modelIdx = (u32)app->models.size() - 1u;

        String directory = GetDirectoryPart(MakeString(filename));

        // Create a list of materials
        u32 baseMeshMaterialIndex = (u32)app->materials.size();
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            app->materials.push_back(Material{});
            Material& material = app->materials.back();
            ProcessAssimpMaterial(app, scene->mMaterials[i], material, directory);
        }

        ProcessAssimpNode(scene, scene->mRootNode, &mesh, baseMeshMaterialIndex, model.materialIdx);

        aiReleaseImport(scene);

        u32 vertexBufferSize = 0;
        u32 indexBufferSize = 0;

        for (u32 i = 0; i < mesh.subMeshes.size(); ++i)
        {
            vertexBufferSize += mesh.subMeshes[i].vertices.size() * sizeof(float);
            indexBufferSize += mesh.subMeshes[i].indices.size() * sizeof(u32);
        }

        glGenBuffers(1, &mesh.vertexBufferHndle);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHndle);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

        glGenBuffers(1, &mesh.indexBufferHndle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vertexBufferHndle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

        u32 indicesOffset = 0;
        u32 verticesOffset = 0;

        for (u32 i = 0; i < mesh.subMeshes.size(); ++i)
        {
            const void* verticesData = mesh.subMeshes[i].vertices.data();
            const u32   verticesSize = mesh.subMeshes[i].vertices.size() * sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
            mesh.subMeshes[i].vertexOffset = verticesOffset;
            verticesOffset += verticesSize;

            const void* indicesData = mesh.subMeshes[i].indices.data();
            const u32   indicesSize = mesh.subMeshes[i].indices.size() * sizeof(u32);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
            mesh.subMeshes[i].indexOffset = indicesOffset;
            indicesOffset += indicesSize;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return modelIdx;
    }
}