/* Engine Copyright (c) 2022 Engine Development Team
   https://github.com/beaumanvienna/gfxRenderEngine

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation files
   (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include <memory>
#include <iostream>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "gtc/type_ptr.hpp"
#include "stb_image.h"

#include "core.h"
#include "VKmodel.h"
#include "renderer/model.h"
#include "auxiliary/hash.h"
#include "auxiliary/file.h"
#include "auxiliary/debug.h"
#include "auxiliary/math.h"
#include "scene/scene.h"

namespace std
{
    template <>
    struct hash<GfxRenderEngine::Vertex>
    {
        size_t operator()(GfxRenderEngine::Vertex const &vertex) const
        {
            size_t seed = 0;
            GfxRenderEngine::HashCombine(seed, vertex.m_Position, vertex.m_Color, vertex.m_Normal, vertex.m_UV);
            return seed;
        }
    };
}

namespace GfxRenderEngine
{
    bool Vertex::operator==(const Vertex& other) const
    {
        return (m_Position    == other.m_Position) &&
               (m_Color       == other.m_Color) &&
               (m_Normal      == other.m_Normal) &&
               (m_UV          == other.m_UV) &&
               (m_DiffuseMapTextureSlot == other.m_DiffuseMapTextureSlot) &&
               (m_Amplification == other.m_Amplification) &&
               (m_Unlit       == other.m_Unlit);
    }

    Builder::Builder(const std::string& filepath)
        : m_Filepath(filepath), m_Transform(nullptr)
    {
        m_Basepath = EngineCore::GetPathWithoutFilename(filepath);
    }

    void Builder::LoadImagesGLTF()
    {
        m_ImageOffset = VK_Model::m_Images.size();
        // retrieve all images from the glTF file
        for (uint i = 0; i < m_GltfModel.images.size(); i++)
        {
            std::string imageFilepath = m_Basepath + m_GltfModel.images[i].uri;
            tinygltf::Image& glTFImage = m_GltfModel.images[i];

            // glTFImage.component - the number of channels in each pixel
            // three channels per pixel need to be converted to four channels per pixel
            uchar* buffer;
            uint64 bufferSize;
            if (glTFImage.component == 3)
            {
                bufferSize = glTFImage.width * glTFImage.height * 4;
                std::vector<uchar> imageData(bufferSize, 0x00);

                buffer = (uchar*)imageData.data();
                uchar* rgba = buffer;
                uchar* rgb = &glTFImage.image[0];
                for (uint i = 0; i < glTFImage.width * glTFImage.height; ++i)
                {
                    memcpy(rgba, rgb, sizeof(uchar) * 3);
                    rgba += 4;
                    rgb += 3;
                }
            }
            else
            {
                buffer = &glTFImage.image[0];
                bufferSize = glTFImage.image.size();
            }
            auto texture = std::make_shared<VK_Texture>(Engine::m_TextureSlotManager);
            texture->Init(glTFImage.width, glTFImage.height, buffer);
            VK_Model::m_Images.push_back(texture);
        }
    }

    void Builder::LoadMaterialsGLTF()
    {
        m_Materials.clear();
        for (uint i = 0; i < m_GltfModel.materials.size(); i++)
        {
            tinygltf::Material glTFMaterial = m_GltfModel.materials[i];

            Material material{};
            material.m_DiffuseColor = glm::vec3(0.5f, 0.5f, 1.0f);
            material.m_Roughness = glTFMaterial.pbrMetallicRoughness.roughnessFactor;
            material.m_Metallic  = glTFMaterial.pbrMetallicRoughness.metallicFactor;
            material.m_NormalMapIntensity = glTFMaterial.normalTexture.scale;

            if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
            {
                material.m_DiffuseColor = glm::make_vec3(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
            }
            if (glTFMaterial.pbrMetallicRoughness.baseColorTexture.index != -1)
            {
                material.m_DiffuseMapIndex = glTFMaterial.pbrMetallicRoughness.baseColorTexture.index;
                material.m_Features |= Material::HAS_DIFFUSE_MAP;
            }
            else if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
            {
                LOG_CORE_WARN("using legacy field values/baseColorTexture");
                material.m_DiffuseMapIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
                material.m_Features |= Material::HAS_DIFFUSE_MAP;
            }
            if (glTFMaterial.normalTexture.index != -1)
            {
                material.m_NormalMapIndex = glTFMaterial.normalTexture.index;
                material.m_Features |= Material::HAS_NORMAL_MAP;
            }
            if (glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
            {
                material.m_RoughnessMettalicMapIndex = glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                material.m_Features |= Material::HAS_ROUGHNESS_METALLIC_MAP;
            }

            m_Materials.push_back(material);
        }
    }

    void Builder::LoadVertexDataGLTF(uint meshIndex)
    {
        // handle vertex data
        m_Vertices.clear();
        m_Indices.clear();
        m_Primitives.clear();

        for (const auto& glTFPrimitive : m_GltfModel.meshes[meshIndex].primitives)
        {
            uint vertexCount = 0;
            uint indexCount  = 0;

            glm::vec3 diffuseColor = glm::vec3(0.5f, 0.5f, 1.0f);
            if (glTFPrimitive.material != -1)
            {
                ASSERT(glTFPrimitive.material < m_Materials.size());
                diffuseColor = m_Materials[glTFPrimitive.material].m_DiffuseColor;
            }
            // Vertices
            {
                const float* positionBuffer  = nullptr;
                const float* normalsBuffer   = nullptr;
                const float* texCoordsBuffer = nullptr;

                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor = m_GltfModel.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& view = m_GltfModel.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float*>(&(m_GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor = m_GltfModel.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& view = m_GltfModel.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float*>(&(m_GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
                {
                    const tinygltf::Accessor& accessor = m_GltfModel.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& view = m_GltfModel.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float*>(&(m_GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++)
                {
                    Vertex vertex{};
                    vertex.m_Amplification      = 1.0f;
                    auto position = glm::make_vec3(&positionBuffer[v * 3]);
                    vertex.m_Position = glm::vec4(position.x, position.y, position.z, 1.0f);
                    vertex.m_Normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vertex.m_UV = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vertex.m_Color = diffuseColor;
                    m_Vertices.push_back(vertex);
                }
            }
            // Indices
            {
                const tinygltf::Accessor& accessor = m_GltfModel.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView& bufferView = m_GltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = m_GltfModel.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                // glTF supports different component types of indices
                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        m_Indices.push_back(buf[index]);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        m_Indices.push_back(buf[index]);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        m_Indices.push_back(buf[index]);
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }

            Primitive primitive;
            primitive.m_FirstVertex = static_cast<uint32_t>(m_Vertices.size());
            primitive.m_VertexCount = vertexCount;
            primitive.m_IndexCount  = indexCount;
            primitive.m_FirstIndex  = static_cast<uint32_t>(m_Indices.size());

            m_Primitives.push_back(primitive);
        }

        // calculate tangents
        CalculateTangents();
    }

    void Builder::LoadTransformationMatrix(TransformComponent& transform, int nodeIndex)
    {
        auto& node = m_GltfModel.nodes[nodeIndex];

        if (node.matrix.size() == 16)
        {
            transform.SetScale(glm::vec3(node.matrix[0], node.matrix[5], node.matrix[10]));
            transform.SetTranslation(glm::vec3(node.matrix[3], node.matrix[7], node.matrix[11]));
        }
        else
        {
            if (node.rotation.size() == 4)
            {
                float x = node.rotation[0];
                float y = node.rotation[1];
                float z = node.rotation[2];
                float w = node.rotation[3];

                transform.SetRotation({w, x, y, z});

            }
            if (node.scale.size() == 3)
            {
                transform.SetScale({node.scale[0], node.scale[1], node.scale[2]});
            }
            if (node.translation.size() == 3)
            {
                transform.SetTranslation({node.translation[0], node.translation[1], node.translation[2]});
            }
        }
    }

    void Builder::AssignMaterial(entt::registry& registry, entt::entity entity, int materialIndex)
    {
        if (materialIndex == -1)
        {
            PbrNoMapComponent pbrNoMapComponent{};
            pbrNoMapComponent.m_Roughness = 0.5;
            pbrNoMapComponent.m_Metallic  = 0.1;
            pbrNoMapComponent.m_Color     = glm::vec3(0.5f, 0.5f, 1.0f);

            registry.emplace<PbrNoMapComponent>(entity, pbrNoMapComponent);
            return;
        }
        ASSERT(materialIndex < m_Materials.size());
        auto& material = m_Materials[materialIndex];

        if (material.m_Features == Material::HAS_DIFFUSE_MAP)
        {
            uint diffuseMapIndex = m_ImageOffset + material.m_DiffuseMapIndex;
            ASSERT(diffuseMapIndex < VK_Model::m_Images.size());
            auto pbrDiffuseComponent = VK_Model::CreateDescriptorSet(VK_Model::m_Images[diffuseMapIndex]);
            pbrDiffuseComponent.m_Roughness                = material.m_Roughness;
            pbrDiffuseComponent.m_Metallic                 = material.m_Metallic;

            registry.emplace<PbrDiffuseComponent>(entity, pbrDiffuseComponent);
        }
        else if (material.m_Features == (Material::HAS_DIFFUSE_MAP | Material::HAS_NORMAL_MAP))
        {
            uint diffuseMapIndex = m_ImageOffset + material.m_DiffuseMapIndex;
            uint normalMapIndex  = m_ImageOffset + material.m_NormalMapIndex;
            ASSERT(diffuseMapIndex < VK_Model::m_Images.size());
            ASSERT(normalMapIndex < VK_Model::m_Images.size());

            auto pbrDiffuseNormalComponent = VK_Model::CreateDescriptorSet(VK_Model::m_Images[diffuseMapIndex], VK_Model::m_Images[normalMapIndex]);
            pbrDiffuseNormalComponent.m_Roughness                = material.m_Roughness;
            pbrDiffuseNormalComponent.m_Metallic                 = material.m_Metallic;
            pbrDiffuseNormalComponent.m_NormalMapIntensity       = material.m_NormalMapIntensity;

            registry.emplace<PbrDiffuseNormalComponent>(entity, pbrDiffuseNormalComponent);
        }
        else if (material.m_Features == (Material::HAS_DIFFUSE_MAP | Material::HAS_NORMAL_MAP | Material::HAS_ROUGHNESS_METALLIC_MAP))
        {
            uint diffuseMapIndex           = m_ImageOffset + material.m_DiffuseMapIndex;
            uint normalMapIndex            = m_ImageOffset + material.m_NormalMapIndex;
            uint roughnessMettalicMapIndex = m_ImageOffset + material.m_RoughnessMettalicMapIndex;
            ASSERT(diffuseMapIndex            < VK_Model::m_Images.size());
            ASSERT(normalMapIndex             < VK_Model::m_Images.size());
            ASSERT(roughnessMettalicMapIndex  < VK_Model::m_Images.size());

            auto pbrDiffuseNormalRoughnessMetallicComponent = 
                VK_Model::CreateDescriptorSet
                (
                    VK_Model::m_Images[diffuseMapIndex], 
                    VK_Model::m_Images[normalMapIndex], 
                    VK_Model::m_Images[roughnessMettalicMapIndex]
                );
            pbrDiffuseNormalRoughnessMetallicComponent.m_NormalMapIntensity       = material.m_NormalMapIntensity;

            registry.emplace<PbrDiffuseNormalRoughnessMetallicComponent>(entity, pbrDiffuseNormalRoughnessMetallicComponent);
        }
        else if (material.m_Features == (Material::HAS_DIFFUSE_MAP | Material::HAS_ROUGHNESS_METALLIC_MAP))
        {
            uint diffuseMapIndex           = m_ImageOffset + material.m_DiffuseMapIndex;
            uint roughnessMettalicMapIndex = m_ImageOffset + material.m_RoughnessMettalicMapIndex;
            ASSERT(diffuseMapIndex            < VK_Model::m_Images.size());
            ASSERT(roughnessMettalicMapIndex  < VK_Model::m_Images.size());

            PbrDiffuseRoughnessMetallicComponent pbrDiffuseRoughnessMetallicComponent{};
            VK_Model::CreateDescriptorSet
            (
                VK_Model::m_Images[diffuseMapIndex],
                VK_Model::m_Images[roughnessMettalicMapIndex],
                pbrDiffuseRoughnessMetallicComponent
            );

            registry.emplace<PbrDiffuseRoughnessMetallicComponent>(entity, pbrDiffuseRoughnessMetallicComponent);
        }
        else if (material.m_Features & (Material::HAS_DIFFUSE_MAP | Material::HAS_NORMAL_MAP | Material::HAS_ROUGHNESS_METALLIC_MAP))
        {
            uint diffuseMapIndex           = m_ImageOffset + material.m_DiffuseMapIndex;
            uint normalMapIndex            = m_ImageOffset + material.m_NormalMapIndex;
            uint roughnessMettalicMapIndex = m_ImageOffset + material.m_RoughnessMettalicMapIndex;
            ASSERT(diffuseMapIndex            < VK_Model::m_Images.size());
            ASSERT(normalMapIndex             < VK_Model::m_Images.size());
            ASSERT(roughnessMettalicMapIndex  < VK_Model::m_Images.size());

            auto pbrDiffuseNormalRoughnessMetallicComponent = 
                VK_Model::CreateDescriptorSet
                (
                    VK_Model::m_Images[diffuseMapIndex], 
                    VK_Model::m_Images[normalMapIndex], 
                    VK_Model::m_Images[roughnessMettalicMapIndex]
                );
            pbrDiffuseNormalRoughnessMetallicComponent.m_NormalMapIntensity       = material.m_NormalMapIntensity;

            registry.emplace<PbrDiffuseNormalRoughnessMetallicComponent>(entity, pbrDiffuseNormalRoughnessMetallicComponent);
        }
        else if (material.m_Features & Material::HAS_DIFFUSE_MAP)
        {
            uint diffuseMapIndex = m_ImageOffset + material.m_DiffuseMapIndex;
            ASSERT(diffuseMapIndex < VK_Model::m_Images.size());
            auto pbrDiffuseComponent = VK_Model::CreateDescriptorSet(VK_Model::m_Images[diffuseMapIndex]);
            pbrDiffuseComponent.m_Roughness                = material.m_Roughness;
            pbrDiffuseComponent.m_Metallic                 = material.m_Metallic;

            registry.emplace<PbrDiffuseComponent>(entity, pbrDiffuseComponent);
        }
        else
        {
            PbrNoMapComponent pbrNoMapComponent{};
            pbrNoMapComponent.m_Roughness = material.m_Roughness;
            pbrNoMapComponent.m_Metallic  = material.m_Metallic;
            pbrNoMapComponent.m_Color     = material.m_DiffuseColor;

            registry.emplace<PbrNoMapComponent>(entity, pbrNoMapComponent);
        }
    }

    void Builder::LoadGLTF(entt::registry& registry, TreeNode& sceneHierarchy, Dictionary& dictionary, TransformComponent* transform)
    {
        std::string warn, err;

        stbi_set_flip_vertically_on_load(false);

        if (!m_GltfLoader.LoadASCIIFromFile(&m_GltfModel, &err, &warn, m_Filepath))
        {
            LOG_CORE_CRITICAL("LoadGLTF errors: {0}, warnings: {1}", err, warn);
        }

        LoadImagesGLTF();
        LoadMaterialsGLTF();

        for (auto& scene : m_GltfModel.scenes)
        {
            TreeNode* currentNode = &sceneHierarchy;
            if (scene.nodes.size() > 1)
            {
                //this scene has multiple nodes -> create a group node
                auto entity = registry.create();
                TransformComponent transform{};
                registry.emplace<TransformComponent>(entity, transform);

                auto shortName = scene.name + "::root";
                auto longName = m_Filepath + std::string("::") + scene.name + std::string("::root");
                TreeNode sceneHierarchyNode{entity, shortName, longName};

                currentNode = currentNode->AddChild(sceneHierarchyNode, dictionary);
            }
            else if (scene.nodes.size() == 1)
            {
                auto& name = m_GltfModel.nodes[scene.nodes[0]].name;
            }
            else
            {
                LOG_CORE_WARN("Builder::LoadGLTF: empty scene in {0}", m_Filepath);
                return;
            }

            for (uint i = 0; i < scene.nodes.size(); i++)
            {
                ProcessNode(scene, scene.nodes[i], registry, dictionary, currentNode);
            }
        }
    }

    void Builder::ProcessNode(tinygltf::Scene& scene, uint nodeIndex, entt::registry& registry, Dictionary& dictionary, TreeNode* currentNode)
    {
        auto& node = m_GltfModel.nodes[nodeIndex];
        auto& nodeName = node.name;
        auto meshIndex = node.mesh;

        if (meshIndex == -1)
        {
            if (node.children.size())
            {
                auto entity = registry.create();
                TransformComponent transform{};
                LoadTransformationMatrix(transform, nodeIndex);
                registry.emplace<TransformComponent>(entity, transform);
                auto shortName = nodeName;
                auto longName = m_Filepath + std::string("::") + scene.name + std::string("::") + nodeName;
                TreeNode sceneHierarchyNode{entity, shortName, longName};

                TreeNode* groupNode = currentNode->AddChild(sceneHierarchyNode, dictionary);
                for (uint childNodeArrayIndex = 0; childNodeArrayIndex < node.children.size(); childNodeArrayIndex++)
                {
                    uint childNodeIndex = node.children[childNodeArrayIndex];
                    ProcessNode(scene, childNodeIndex, registry, dictionary, groupNode);
                }
            }
            else
            {
                LOG_CORE_WARN("No mesh and no children for node {0} in scene {1}, file {2}", nodeName, scene.name, m_Filepath);
            }
        }
        else
        {
            auto newNode = CreateGameObject(scene, nodeIndex, registry, dictionary, currentNode);
            if (node.children.size())
            {
                for (uint childNodeArrayIndex = 0; childNodeArrayIndex < node.children.size(); childNodeArrayIndex++)
                {
                    uint childNodeIndex = node.children[childNodeArrayIndex];
                    ProcessNode(scene, childNodeIndex, registry, dictionary, newNode);
                }
            }
        }
    }

    TreeNode* Builder::CreateGameObject(tinygltf::Scene& scene, uint nodeIndex, entt::registry& registry, Dictionary& dictionary, TreeNode* currentNode)
    {
        auto& node = m_GltfModel.nodes[nodeIndex];
        auto& nodeName = node.name;
        uint meshIndex = node.mesh;

        LoadVertexDataGLTF(meshIndex);
        LOG_CORE_INFO("Vertex count: {0}, Index count: {1} (file: {2}, node: {3})", m_Vertices.size(), m_Indices.size(), m_Filepath, nodeName);

        auto model = Engine::m_Engine->LoadModel(*this);
        auto entity = registry.create();

        auto longName = m_Filepath + std::string("::") + scene.name + std::string("::") + nodeName;

        TreeNode sceneHierarchyNode{entity, nodeName, longName};
        TreeNode* newNode = currentNode->AddChild(sceneHierarchyNode, dictionary);

        // mesh
        MeshComponent mesh{nodeName, model};
        registry.emplace<MeshComponent>(entity, mesh);

        // transform
        TransformComponent transform{};
        LoadTransformationMatrix(transform, nodeIndex);
        registry.emplace<TransformComponent>(entity, transform);

        // material
        auto materialIndex = m_GltfModel.meshes[meshIndex].primitives[0].material;
        AssignMaterial(registry, entity, materialIndex);

        return newNode;
    }

    void Builder::LoadModel(const std::string &filepath, int diffuseMapTextureSlot, int fragAmplification, int normalTextureSlot)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
        {
            LOG_CORE_CRITICAL("LoadModel errors: {0}, warnings: {1}", err, warn);
        }

        m_Vertices.clear();
        m_Indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};
                vertex.m_DiffuseMapTextureSlot = diffuseMapTextureSlot;
                vertex.m_Amplification      = fragAmplification;

                if (index.vertex_index >= 0)
                {
                    vertex.m_Position =
                    {
                        attrib.vertices[3 * index.vertex_index + 0],
                       -attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                    };

                    vertex.m_Color =
                    {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2],
                    };
                }

                if (index.normal_index >= 0)
                {
                    vertex.m_Normal =
                    {
                        attrib.normals[3 * index.normal_index + 0],
                       -attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }

                if (index.texcoord_index >= 0)
                {
                    vertex.m_UV =
                    {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint>(m_Vertices.size());
                    m_Vertices.push_back(vertex);
                }
                m_Indices.push_back(uniqueVertices[vertex]);

            }
        }
        // calculate tangents
        CalculateTangents();
        LOG_CORE_INFO("Vertex count: {0}, Index count: {1} ({2})", m_Vertices.size(), m_Indices.size(), filepath);
    }

    void Builder::CalculateTangents()
    {
        uint cnt = 0;
        uint vertexIndex1;
        uint vertexIndex2;
        uint vertexIndex3;
        glm::vec3 position1;
        glm::vec3 position2;
        glm::vec3 position3;
        glm::vec2 uv1;
        glm::vec2 uv2;
        glm::vec2 uv3;

        for (uint index : m_Indices)
        {
            auto& vertex = m_Vertices[index];

            switch (cnt)
            {
                case 0:
                    position1 = vertex.m_Position;
                    uv1  = vertex.m_UV;
                    vertexIndex1 = index;
                    break;
                case 1:
                    position2 = vertex.m_Position;
                    uv2  = vertex.m_UV;
                    vertexIndex2 = index;
                    break;
                case 2:
                    position3 = vertex.m_Position;
                    uv3  = vertex.m_UV;
                    vertexIndex3 = index;

                    glm::vec3 edge1 = position2 - position1;
                    glm::vec3 edge2 = position3 - position1;
                    glm::vec2 deltaUV1 = uv2 - uv1;
                    glm::vec2 deltaUV2 = uv3 - uv1;

                    float dU1 = deltaUV1.x;
                    float dU2 = deltaUV2.x;
                    float dV1 = deltaUV1.y;
                    float dV2 = deltaUV2.y;
                    float E1x = edge1.x;
                    float E2x = edge2.x;
                    float E1y = edge1.y;
                    float E2y = edge2.y;
                    float E1z = edge1.z;
                    float E2z = edge2.z;

                    float factor = 1.0f / (dU1 * dV2 - dU2 * dV1);

                    glm::vec3 tangent;

                    tangent.x = factor * (dV2 * E1x - dV1 * E2x);
                    tangent.y = factor * (dV2 * E1y - dV1 * E2y);
                    tangent.z = factor * (dV2 * E1z - dV1 * E2z);

                    m_Vertices[vertexIndex1].m_Tangent = tangent;
                    m_Vertices[vertexIndex2].m_Tangent = tangent;
                    m_Vertices[vertexIndex3].m_Tangent = tangent;

                    break;
            }
            cnt = (cnt + 1) % 3;
        }
    }

    void Builder::LoadSprite(Sprite* sprite, const glm::mat4& position, float amplification, int unlit, const glm::vec4& color)
    {
        m_Vertices.clear();
        m_Indices.clear();

        // 0 - 1
        // | / |
        // 3 - 2
        int slot = sprite->GetTextureSlot();

        Vertex vertex[4]
        {
            // index 0, 0.0f,  1.0f
            {/*pos*/ {position[0]}, /*col*/ {0.0f, 0.1f, 0.9f}, /*norm*/ {0.0f, 0.0f, 1.0f}, /*uv*/ {sprite->m_Pos1X, 1.0f-sprite->m_Pos2Y}, slot, amplification, unlit},

            // index 1, 1.0f,  1.0f
            {/*pos*/ {position[1]}, /*col*/ {0.0f, 0.1f, 0.9f}, /*norm*/ {0.0f, 0.0f, 1.0f}, /*uv*/ {sprite->m_Pos2X, 1.0f-sprite->m_Pos2Y}, slot, amplification, unlit},

            // index 2, 1.0f,  0.0f
            {/*pos*/ {position[2]}, /*col*/ {0.0f, 0.9f, 0.1f}, /*norm*/ {0.0f, 0.0f, 1.0f}, /*uv*/ {sprite->m_Pos2X, 1.0f-sprite->m_Pos1Y}, slot, amplification, unlit},

            // index 3, 0.0f,  0.0f
            {/*pos*/ {position[3]}, /*col*/ {0.0f, 0.9f, 0.1f}, /*norm*/ {0.0f, 0.0f, 1.0f}, /*uv*/ {sprite->m_Pos1X, 1.0f-sprite->m_Pos1Y}, slot, amplification, unlit}
        };
        for (int i = 0; i < 4; i++) m_Vertices.push_back(vertex[i]);
        slot++;

        m_Indices.push_back(0);
        m_Indices.push_back(1);
        m_Indices.push_back(3);
        m_Indices.push_back(1);
        m_Indices.push_back(2);
        m_Indices.push_back(3);
    }

    void Builder::LoadParticle(const glm::vec4& color)
    {
        m_Vertices.clear();
        m_Indices.clear();

        // 0 - 1
        // | / |
        // 3 - 2

        Vertex vertex[4]
        {
            // index 0, 0.0f,  1.0f
            {/*pos*/ {-1.0f,  1.0f, 0.0f}, {color.x, color.y, color.z}, /*norm*/ {0.0f, 0.0f, -1.0f}, /*uv*/ {0.0f, 1.0f-1.0f}, /*slot*/0, 1.0f /*amplification*/, 0 /*unlit*/},

            // index 1, 1.0f,  1.0f
            {/*pos*/ { 1.0f,  1.0f, 0.0f}, {color.x, color.y, color.z}, /*norm*/ {0.0f, 0.0f, -1.0f}, /*uv*/ {1.0f, 1.0f-1.0f}, /*slot*/0, 1.0f /*amplification*/, 0 /*unlit*/},

            // index 2, 1.0f,  0.0f
            {/*pos*/ { 1.0f, -1.0f, 0.0f}, {color.x, color.y, color.z}, /*norm*/ {0.0f, 0.0f, -1.0f}, /*uv*/ {1.0f, 1.0f-0.0f}, /*slot*/0, 1.0f /*amplification*/, 0 /*unlit*/},

            // index 3, 0.0f,  0.0f
            {/*pos*/ {-1.0f, -1.0f, 0.0f}, {color.x, color.y, color.z}, /*norm*/ {0.0f, 0.0f, -1.0f}, /*uv*/ {0.0f, 1.0f-0.0f}, /*slot*/0, 1.0f /*amplification*/, 0 /*unlit*/}
        };
        for (int i = 0; i < 4; i++) m_Vertices.push_back(vertex[i]);

        m_Indices.push_back(0);
        m_Indices.push_back(1);
        m_Indices.push_back(3);
        m_Indices.push_back(1);
        m_Indices.push_back(2);
        m_Indices.push_back(3);
    }
}
