#pragma once
#ifndef SCENE_HPP
#define SCENE_HPP

#include "xac.hpp"
#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <cstring>

namespace scene
{

    // ------------------ Math ------------------

    struct Vector2
    {
        float x{}, y{};
    };
    struct Vector3
    {
        float x{}, y{}, z{};
    };
    struct Vector4
    {
        float x{}, y{}, z{}, w{};
    };
    struct RGBAColor
    {
        float r{}, g{}, b{}, a{};
    };

    // ------------------ DX to GL conversion helpers ------------------

    inline Vector3 dx_to_gl_position(const Vector3 &v) { return {v.x, v.y, -v.z}; }
    inline Vector3 dx_to_gl_scale(const Vector3 &v) { return {v.x, v.y, v.z}; } // scale does NOT flip
    inline Vector4 dx_to_gl_quat(const Vector4 &q) { return {-q.x, q.y, q.z, -q.w}; }

    // ------------------ Mesh ------------------

    struct SubMesh
    {
        std::string name;
        std::string textures;

        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<Vector4> tangents;
        std::vector<Vector3> bitangents;
        std::vector<Vector2> uvcoords;

        std::vector<uint32_t> colors32;
        std::vector<RGBAColor> colors128;

        std::vector<uint32_t> original_vertex_numbers;
        std::vector<uint32_t> indices;
    };

    struct Model
    {
        std::string name;
        std::vector<SubMesh> submeshes;
    };

    // ------------------ Scene Graph ------------------

    struct SceneNode
    {
        std::string name;
        std::optional<Vector3> position;
        std::optional<Vector4> rotation;
        std::optional<Vector3> scale;

        std::optional<Model> model;
        std::vector<SceneNode> children;
    };

    struct Scene
    {
        std::vector<SceneNode> root_nodes;
        std::optional<Vector3> position;
        std::optional<Vector4> rotation;
        std::optional<Vector3> scale;

        // ------------------ API ------------------

        static Scene from_xac_root(const xac::XACRoot &xac_root,
                                   const std::string &texture_path)
        {
            Scene scene;

            auto textures = xac_root.get_texture_names_with_path(texture_path);

            for (const auto &entry : xac_root.chunks)
            {
                if (entry.chunk.chunk_id != static_cast<uint32_t>(xac::XACChunkId::Mesh))
                    continue;

                std::optional<Model> model;

                if (entry.chunk.version == 1)
                {
                    if (auto *mesh = std::get_if<xac::XACMesh>(&entry.chunk_data))
                    {
                        model = Model{
                            "", // keep empty like Rust
                            parse_vertex_data(mesh->vertex_attribute_layer,
                                              mesh->sub_meshes,
                                              textures)};
                    }
                }
                else if (entry.chunk.version == 2)
                {
                    if (auto *mesh = std::get_if<xac::XACMesh2>(&entry.chunk_data))
                    {
                        model = Model{
                            "", // keep empty like Rust
                            parse_vertex_data(mesh->vertex_attribute_layer,
                                              mesh->sub_meshes,
                                              textures)};
                    }
                }

                if (model)
                {
                    SceneNode node;
                    node.name = model->name;
                    node.model = std::move(model);
                    scene.root_nodes.push_back(std::move(node));
                }
            }

            return scene;
        }

    private:
        // ------------------ Safe read helpers ------------------

        static float read_f32(const uint8_t *data)
        {
            float v;
            std::memcpy(&v, data, sizeof(float));
            return v;
        }

        static uint32_t read_u32(const uint8_t *data)
        {
            uint32_t v;
            std::memcpy(&v, data, sizeof(uint32_t));
            return v;
        }

        // ------------------ Vertex parsing ------------------

        static std::vector<SubMesh> parse_vertex_data(
            const std::vector<xac::XACVertexAttributeLayer> &layers,
            const std::vector<xac::XACSubMesh> &submeshes,
            const std::vector<std::string> &textures_data)
        {
            auto find_layer = [&](uint32_t id) -> const xac::XACVertexAttributeLayer *
            {
                for (auto &l : layers)
                    if (l.layer_type_id == id)
                        return &l;
                return nullptr;
            };

            const auto *positions = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribPositions));
            const auto *normals = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribNormals));
            const auto *tangents = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribTangents));
            const auto *uvs = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribUvcoords));
            const auto *colors32 = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribColors32));
            const auto *colors128 = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribColors128));
            const auto *bitangents = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribBitangents));
            const auto *orig_vtx = find_layer(static_cast<uint32_t>(xac::XACAttribute::AttribOrgvtxnumbers));

            size_t vertex_offset = 0;
            std::vector<SubMesh> result;

            for (const auto &sub : submeshes)
            {
                SubMesh s;

                // Assign texture
                if (sub.material_index != 0 && sub.material_index < textures_data.size())
                    s.textures = textures_data[sub.material_index];

                // Indices
                for (uint32_t i = 0; i < sub.num_indices; ++i)
                    s.indices.push_back(sub.indices[i]);

                // Vertices
                for (uint32_t v = 0; v < sub.num_verts; ++v)
                {
                    size_t i = vertex_offset + v;

                    if (positions)
                    {
                        auto &data = positions->mesh_data;
                        size_t off = i * 12;
                        if (off + 12 <= data.size())
                        {
                            float x = read_f32(&data[off]);
                            float y = read_f32(&data[off + 4]);
                            float z = read_f32(&data[off + 8]);
                            s.positions.push_back(dx_to_gl_position({x, y, z}));
                        }
                    }

                    if (normals)
                    {
                        auto &data = normals->mesh_data;
                        size_t off = i * 12;
                        if (off + 12 <= data.size())
                        {
                            float x = read_f32(&data[off]);
                            float y = read_f32(&data[off + 4]);
                            float z = read_f32(&data[off + 8]);
                            s.normals.push_back(dx_to_gl_position({x, y, z}));
                        }
                    }

                    if (tangents)
                    {
                        auto &data = tangents->mesh_data;
                        size_t off = i * 16;
                        if (off + 16 <= data.size())
                        {
                            float x = read_f32(&data[off]);
                            float y = read_f32(&data[off + 4]);
                            float z = read_f32(&data[off + 8]);
                            float w = read_f32(&data[off + 12]);
                            s.tangents.push_back({x, y, z, w});
                        }
                    }

                    if (uvs)
                    {
                        auto &data = uvs->mesh_data;
                        size_t off = i * 8;
                        if (off + 8 <= data.size())
                        {
                            float u = read_f32(&data[off]);
                            float v_ = read_f32(&data[off + 4]);
                            s.uvcoords.push_back({u, 1.0f - v_});
                        }
                    }

                    if (colors32)
                    {
                        auto &data = colors32->mesh_data;
                        size_t off = i * 4;
                        if (off + 4 <= data.size())
                            s.colors32.push_back(read_u32(&data[off]));
                    }

                    if (colors128)
                    {
                        auto &data = colors128->mesh_data;
                        size_t off = i * 16;
                        if (off + 16 <= data.size())
                        {
                            float r = read_f32(&data[off]);
                            float g = read_f32(&data[off + 4]);
                            float b = read_f32(&data[off + 8]);
                            float a = read_f32(&data[off + 12]);
                            s.colors128.push_back({r, g, b, a});
                        }
                    }

                    if (bitangents)
                    {
                        auto &data = bitangents->mesh_data;
                        size_t off = i * 12;
                        if (off + 12 <= data.size())
                        {
                            float x = read_f32(&data[off]);
                            float y = read_f32(&data[off + 4]);
                            float z = read_f32(&data[off + 8]);
                            s.bitangents.push_back({x, y, z});
                        }
                    }

                    if (orig_vtx)
                    {
                        auto &data = orig_vtx->mesh_data;
                        size_t off = i * 4;
                        if (off + 4 <= data.size())
                            s.original_vertex_numbers.push_back(read_u32(&data[off]));
                    }
                }

                vertex_offset += sub.num_verts;
                result.push_back(std::move(s));
            }

            return result;
        }
    };

} // namespace scene

#endif