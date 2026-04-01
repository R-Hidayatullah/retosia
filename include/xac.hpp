#pragma once
#ifndef XAC_HPP
#define XAC_HPP

// ============================================================
//  xac.hpp  –  Actor file parser (C++20 port of xac.rs)
// ============================================================
//  XAC files describe 3D actors: meshes, materials, skeleton nodes,
//  skinning data, morph targets, and FX material parameters.
//
//  Chunk layout (little-endian):
//    XACHeader  (8 bytes)
//    Repeated:  FileChunk header (12 bytes) + chunk payload
//
//  Dependencies: binary_reader.hpp
//  Requires:     C++20
// ============================================================

#include "binary_reader.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace xac
{

    // -------------------------------------------------------
    //  Chunk ID constants
    // -------------------------------------------------------
    enum class XACChunkId : uint32_t
    {
        Node = 0,
        Mesh = 1,
        SkinningInfo = 2,
        StdMaterial = 3,
        StdMaterialLayer = 4,
        FXMaterial = 5,
        Limit = 6,
        Info = 7,
        MeshLodLevels = 8,
        StdProgMorphTarget = 9,
        NodeGroups = 10,
        Nodes = 11,
        StdPMorphTargets = 12,
        MaterialInfo = 13,
        NodeMotionSources = 14,
        AttachmentNodes = 15,
    };

    // -------------------------------------------------------
    //  Helpers
    // -------------------------------------------------------
    namespace detail
    {
        inline std::string read_len_str(binreader::BinaryReader &r)
        {
            uint32_t len = r.read<uint32_t>();
            if (len == 0)
                return {};
            std::vector<uint8_t> buf(len);
            r.read_bytes(buf.data(), len);
            return std::string(buf.begin(), buf.end());
        }
    }

    // -------------------------------------------------------
    //  Shared math / primitive types
    // -------------------------------------------------------
    struct FileColor
    {
        float r{}, g{}, b{}, a{};
        void read(binreader::BinaryReader &rr)
        {
            r = rr.read<float>();
            g = rr.read<float>();
            b = rr.read<float>();
            a = rr.read<float>();
        }
    };

    struct FileVector3
    {
        float x{}, y{}, z{};
        void read(binreader::BinaryReader &rr)
        {
            x = rr.read<float>();
            y = rr.read<float>();
            z = rr.read<float>();
        }
    };

    struct File16BitVector3
    {
        uint16_t x{}, y{}, z{};
        void read(binreader::BinaryReader &rr)
        {
            x = rr.read<uint16_t>();
            y = rr.read<uint16_t>();
            z = rr.read<uint16_t>();
        }
    };

    struct File8BitVector3
    {
        uint8_t x{}, y{}, z{};
        void read(binreader::BinaryReader &rr)
        {
            x = rr.read<uint8_t>();
            y = rr.read<uint8_t>();
            z = rr.read<uint8_t>();
        }
    };

    struct FileQuaternion
    {
        float x{}, y{}, z{}, w{};
        void read(binreader::BinaryReader &rr)
        {
            x = rr.read<float>();
            y = rr.read<float>();
            z = rr.read<float>();
            w = rr.read<float>();
        }
    };

    struct File16BitQuaternion
    {
        int16_t x{}, y{}, z{}, w{};
        void read(binreader::BinaryReader &rr)
        {
            x = rr.read<int16_t>();
            y = rr.read<int16_t>();
            z = rr.read<int16_t>();
            w = rr.read<int16_t>();
        }
    };

    // -------------------------------------------------------
    //  File chunk header
    // -------------------------------------------------------
    struct FileChunk
    {
        uint32_t chunk_id{0};
        uint32_t size_in_bytes{0};
        uint32_t version{0};

        void read(binreader::BinaryReader &r)
        {
            chunk_id = r.read<uint32_t>();
            size_in_bytes = r.read<uint32_t>();
            version = r.read<uint32_t>();
        }
    };

    // -------------------------------------------------------
    //  XAC file header
    // -------------------------------------------------------
    struct XACHeader
    {
        uint32_t fourcc{0}; // "XAC "
        uint8_t hi_version{0};
        uint8_t lo_version{0};
        uint8_t endian_type{0};
        uint8_t mul_order{0};

        void read(binreader::BinaryReader &r)
        {
            fourcc = r.read<uint32_t>();
            hi_version = r.read<uint8_t>();
            lo_version = r.read<uint8_t>();
            endian_type = r.read<uint8_t>();
            mul_order = r.read<uint8_t>();
        }

        bool is_valid() const
        {
            // "XAC " little-endian = 0x20434158
            return (fourcc & 0x00FFFFFF) == 0x00434158;
        }
    };

    // -------------------------------------------------------
    //  INFO chunks (versions 1-4)
    // -------------------------------------------------------
    struct XACInfo
    {
        uint32_t repositioning_mask{0};
        uint32_t repositioning_node_index{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string actor_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            repositioning_mask = r.read<uint32_t>();
            repositioning_node_index = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            actor_name = read_len_str(r);
        }
    };

    struct XACInfo2
    {
        uint32_t repositioning_mask{0};
        uint32_t repositioning_node_index{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        float retarget_root_offset{0.0f};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string actor_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            repositioning_mask = r.read<uint32_t>();
            repositioning_node_index = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            retarget_root_offset = r.read<float>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            actor_name = read_len_str(r);
        }
    };

    struct XACInfo3
    {
        uint32_t trajectory_node_index{0};
        uint32_t motion_extraction_node_index{0};
        uint32_t motion_extraction_mask{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        float retarget_root_offset{0.0f};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string actor_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            trajectory_node_index = r.read<uint32_t>();
            motion_extraction_node_index = r.read<uint32_t>();
            motion_extraction_mask = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            retarget_root_offset = r.read<float>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            actor_name = read_len_str(r);
        }
    };

    struct XACInfo4
    {
        uint32_t num_lods{0};
        uint32_t trajectory_node_index{0};
        uint32_t motion_extraction_node_index{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        float retarget_root_offset{0.0f};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string actor_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            num_lods = r.read<uint32_t>();
            trajectory_node_index = r.read<uint32_t>();
            motion_extraction_node_index = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            retarget_root_offset = r.read<float>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            actor_name = read_len_str(r);
        }
    };

    // -------------------------------------------------------
    //  Node chunks (versions 1-4)
    // -------------------------------------------------------
    struct XACNode
    {
        FileQuaternion local_quat;
        FileQuaternion scale_rot;
        FileVector3 local_pos;
        FileVector3 local_scale;
        FileVector3 shear;
        uint32_t skeletal_lods{0};
        uint32_t parent_index{0};
        std::string node_name;

        void read(binreader::BinaryReader &r)
        {
            local_quat.read(r);
            scale_rot.read(r);
            local_pos.read(r);
            local_scale.read(r);
            shear.read(r);
            skeletal_lods = r.read<uint32_t>();
            parent_index = r.read<uint32_t>();
            node_name = detail::read_len_str(r);
        }
    };

    struct XACNode2 : XACNode
    {
        uint8_t node_flags{0};
        uint8_t padding[3]{};

        void read(binreader::BinaryReader &r)
        {
            local_quat.read(r);
            scale_rot.read(r);
            local_pos.read(r);
            local_scale.read(r);
            shear.read(r);
            skeletal_lods = r.read<uint32_t>();
            parent_index = r.read<uint32_t>();
            node_flags = r.read<uint8_t>();
            r.read_bytes(padding, 3);
            node_name = detail::read_len_str(r);
        }
    };

    struct XACNode3 : XACNode2
    {
        float obb[16]{}; // Oriented Bounding Box

        void read(binreader::BinaryReader &r)
        {
            local_quat.read(r);
            scale_rot.read(r);
            local_pos.read(r);
            local_scale.read(r);
            shear.read(r);
            skeletal_lods = r.read<uint32_t>();
            parent_index = r.read<uint32_t>();
            node_flags = r.read<uint8_t>();
            for (auto &f : obb)
                f = r.read<float>();
            r.read_bytes(padding, 3);
            node_name = detail::read_len_str(r);
        }
    };

    struct XACNode4
    {
        FileQuaternion local_quat;
        FileQuaternion scale_rot;
        FileVector3 local_pos;
        FileVector3 local_scale;
        FileVector3 shear;
        uint32_t skeletal_lods{0};
        uint32_t motion_lods{0};
        uint32_t parent_index{0};
        uint32_t num_children{0};
        uint8_t node_flags{0};
        float obb[16]{};
        float importance_factor{0.0f};
        std::string node_name;

        void read(binreader::BinaryReader &r)
        {
            local_quat.read(r);
            scale_rot.read(r);
            local_pos.read(r);
            local_scale.read(r);
            shear.read(r);
            skeletal_lods = r.read<uint32_t>();
            motion_lods = r.read<uint32_t>();
            parent_index = r.read<uint32_t>();
            num_children = r.read<uint32_t>();
            node_flags = r.read<uint8_t>();
            for (auto &f : obb)
                f = r.read<float>();
            importance_factor = r.read<float>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            node_name = detail::read_len_str(r);
        }
    };

    // -------------------------------------------------------
    //  Material types
    // -------------------------------------------------------
    struct XACStandardMaterial
    {
        FileColor ambient;
        FileColor diffuse;
        FileColor specular;
        FileColor emissive;
        float shine{0.0f};
        float shine_strength{0.0f};
        float opacity{1.0f};
        float ior{1.0f};
        uint8_t double_sided{0};
        uint8_t wireframe{0};
        uint8_t transparency_type{0};
        uint8_t padding{0};
        std::string material_name;

        void read(binreader::BinaryReader &r)
        {
            ambient.read(r);
            diffuse.read(r);
            specular.read(r);
            emissive.read(r);
            shine = r.read<float>();
            shine_strength = r.read<float>();
            opacity = r.read<float>();
            ior = r.read<float>();
            double_sided = r.read<uint8_t>();
            wireframe = r.read<uint8_t>();
            transparency_type = r.read<uint8_t>();
            padding = r.read<uint8_t>();
            material_name = detail::read_len_str(r);
        }
    };

    struct XACStandardMaterialLayer2;

    struct XACStandardMaterial2 : XACStandardMaterial
    {
        uint8_t num_layers{0};
        std::vector<XACStandardMaterialLayer2> standard_material_layer2;

        void read(binreader::BinaryReader &r); // defined after Layer2
    };

    struct XACStandardMaterial3
    {
        uint32_t lod{0};
        FileColor ambient;
        FileColor diffuse;
        FileColor specular;
        FileColor emissive;
        float shine{0.0f};
        float shine_strength{0.0f};
        float opacity{1.0f};
        float ior{1.0f};
        uint8_t double_sided{0};
        uint8_t wireframe{0};
        uint8_t transparency_type{0};
        uint8_t num_layers{0};
        std::string material_name;
        std::vector<XACStandardMaterialLayer2> standard_material_layer2;

        void read(binreader::BinaryReader &r); // defined after Layer2
    };

    struct XACStandardMaterialLayer
    {
        float amount{0.0f};
        float u_offset{0.0f};
        float v_offset{0.0f};
        float u_tiling{1.0f};
        float v_tiling{1.0f};
        float rotation_radians{0.0f};
        uint16_t material_number{0};
        uint8_t map_type{0};
        uint8_t padding{0};
        std::string texture_name;

        void read(binreader::BinaryReader &r)
        {
            amount = r.read<float>();
            u_offset = r.read<float>();
            v_offset = r.read<float>();
            u_tiling = r.read<float>();
            v_tiling = r.read<float>();
            rotation_radians = r.read<float>();
            material_number = r.read<uint16_t>();
            map_type = r.read<uint8_t>();
            padding = r.read<uint8_t>();
            texture_name = detail::read_len_str(r);
        }
    };

    struct XACStandardMaterialLayer2
    {
        float amount{0.0f};
        float u_offset{0.0f};
        float v_offset{0.0f};
        float u_tiling{1.0f};
        float v_tiling{1.0f};
        float rotation_radians{0.0f};
        uint16_t material_number{0};
        uint8_t map_type{0};
        uint8_t blend_mode{0};
        std::string texture_name;

        void read(binreader::BinaryReader &r)
        {
            amount = r.read<float>();
            u_offset = r.read<float>();
            v_offset = r.read<float>();
            u_tiling = r.read<float>();
            v_tiling = r.read<float>();
            rotation_radians = r.read<float>();
            material_number = r.read<uint16_t>();
            map_type = r.read<uint8_t>();
            blend_mode = r.read<uint8_t>();
            texture_name = detail::read_len_str(r);
        }
    };

    // Deferred method bodies
    inline void XACStandardMaterial2::read(binreader::BinaryReader &r)
    {
        ambient.read(r);
        diffuse.read(r);
        specular.read(r);
        emissive.read(r);
        shine = r.read<float>();
        shine_strength = r.read<float>();
        opacity = r.read<float>();
        ior = r.read<float>();
        double_sided = r.read<uint8_t>();
        wireframe = r.read<uint8_t>();
        transparency_type = r.read<uint8_t>();
        num_layers = r.read<uint8_t>();
        material_name = detail::read_len_str(r);
        standard_material_layer2.resize(num_layers);
        for (auto &l : standard_material_layer2)
            l.read(r);
    }

    inline void XACStandardMaterial3::read(binreader::BinaryReader &r)
    {
        lod = r.read<uint32_t>();
        ambient.read(r);
        diffuse.read(r);
        specular.read(r);
        emissive.read(r);
        shine = r.read<float>();
        shine_strength = r.read<float>();
        opacity = r.read<float>();
        ior = r.read<float>();
        double_sided = r.read<uint8_t>();
        wireframe = r.read<uint8_t>();
        transparency_type = r.read<uint8_t>();
        num_layers = r.read<uint8_t>();
        material_name = detail::read_len_str(r);
        standard_material_layer2.resize(num_layers);
        for (auto &l : standard_material_layer2)
            l.read(r);
    }

    // -------------------------------------------------------
    //  Vertex attribute layer
    // -------------------------------------------------------
    struct XACVertexAttributeLayer
    {
        uint32_t layer_type_id{0};
        uint32_t attrib_size_in_bytes{0};
        uint8_t enable_deformations{0};
        uint8_t is_scale{0};
        std::vector<uint8_t> mesh_data;

        void read(binreader::BinaryReader &r, uint32_t total_verts)
        {
            layer_type_id = r.read<uint32_t>();
            attrib_size_in_bytes = r.read<uint32_t>();
            enable_deformations = r.read<uint8_t>();
            is_scale = r.read<uint8_t>();
            r.read<uint16_t>(); // padding
            std::size_t sz = attrib_size_in_bytes * total_verts;
            mesh_data.resize(sz);
            r.read_bytes(mesh_data.data(), sz);
        }
    };

    // -------------------------------------------------------
    //  Sub-mesh
    // -------------------------------------------------------
    struct XACSubMesh
    {
        uint32_t num_indices{0};
        uint32_t num_verts{0};
        uint32_t material_index{0};
        uint32_t num_bones{0};
        std::vector<uint32_t> indices;
        std::vector<uint32_t> bones;

        void read(binreader::BinaryReader &r)
        {
            num_indices = r.read<uint32_t>();
            num_verts = r.read<uint32_t>();
            material_index = r.read<uint32_t>();
            num_bones = r.read<uint32_t>();
            indices.resize(num_indices);
            for (auto &i : indices)
                i = r.read<uint32_t>();
            bones.resize(num_bones);
            for (auto &b : bones)
                b = r.read<uint32_t>();
        }
    };

    // -------------------------------------------------------
    //  Mesh chunks (versions 1-2)
    // -------------------------------------------------------
    struct XACMesh
    {
        uint32_t node_index{0};
        uint32_t num_org_verts{0};
        uint32_t total_verts{0};
        uint32_t total_indices{0};
        uint32_t num_sub_meshes{0};
        uint32_t num_layers{0};
        uint8_t is_collision_mesh{0};
        std::vector<XACVertexAttributeLayer> vertex_attribute_layer;
        std::vector<XACSubMesh> sub_meshes;

        void read(binreader::BinaryReader &r)
        {
            node_index = r.read<uint32_t>();
            num_org_verts = r.read<uint32_t>();
            total_verts = r.read<uint32_t>();
            total_indices = r.read<uint32_t>();
            num_sub_meshes = r.read<uint32_t>();
            num_layers = r.read<uint32_t>();
            is_collision_mesh = r.read<uint8_t>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            vertex_attribute_layer.resize(num_layers);
            for (auto &l : vertex_attribute_layer)
                l.read(r, total_verts);
            sub_meshes.resize(num_sub_meshes);
            for (auto &s : sub_meshes)
                s.read(r);
        }
    };

    struct XACMesh2
    {
        uint32_t node_index{0};
        uint32_t lod{0};
        uint32_t num_org_verts{0};
        uint32_t total_verts{0};
        uint32_t total_indices{0};
        uint32_t num_sub_meshes{0};
        uint32_t num_layers{0};
        uint8_t is_collision_mesh{0};
        std::vector<XACVertexAttributeLayer> vertex_attribute_layer;
        std::vector<XACSubMesh> sub_meshes;

        void read(binreader::BinaryReader &r)
        {
            node_index = r.read<uint32_t>();
            lod = r.read<uint32_t>();
            num_org_verts = r.read<uint32_t>();
            total_verts = r.read<uint32_t>();
            total_indices = r.read<uint32_t>();
            num_sub_meshes = r.read<uint32_t>();
            num_layers = r.read<uint32_t>();
            is_collision_mesh = r.read<uint8_t>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            vertex_attribute_layer.resize(num_layers);
            for (auto &l : vertex_attribute_layer)
                l.read(r, total_verts);
            sub_meshes.resize(num_sub_meshes);
            for (auto &s : sub_meshes)
                s.read(r);
        }
    };

    // -------------------------------------------------------
    //  Limit chunk
    // -------------------------------------------------------
    struct XACLimit
    {
        FileVector3 translation_min;
        FileVector3 translation_max;
        FileVector3 rotation_min;
        FileVector3 rotation_max;
        FileVector3 scale_min;
        FileVector3 scale_max;
        uint8_t limit_flags[9]{};
        uint32_t node_number{0};

        void read(binreader::BinaryReader &r)
        {
            translation_min.read(r);
            translation_max.read(r);
            rotation_min.read(r);
            rotation_max.read(r);
            scale_min.read(r);
            scale_max.read(r);
            r.read_bytes(limit_flags, 9);
            node_number = r.read<uint32_t>();
        }
    };

    // -------------------------------------------------------
    //  Morph target chunks
    // -------------------------------------------------------
    struct XACPMorphTargetMeshDeltas
    {
        uint32_t node_index{0};
        float min_value{0.0f};
        float max_value{0.0f};
        uint32_t num_vertices{0};
        std::vector<File16BitVector3> delta_position_values;
        std::vector<File8BitVector3> delta_normal_values;
        std::vector<File8BitVector3> delta_tangent_values;
        std::vector<uint32_t> vertex_numbers;

        void read(binreader::BinaryReader &r)
        {
            node_index = r.read<uint32_t>();
            min_value = r.read<float>();
            max_value = r.read<float>();
            num_vertices = r.read<uint32_t>();
            delta_position_values.resize(num_vertices);
            for (auto &v : delta_position_values)
                v.read(r);
            delta_normal_values.resize(num_vertices);
            for (auto &v : delta_normal_values)
                v.read(r);
            delta_tangent_values.resize(num_vertices);
            for (auto &v : delta_tangent_values)
                v.read(r);
            vertex_numbers.resize(num_vertices);
            for (auto &v : vertex_numbers)
                v = r.read<uint32_t>();
        }
    };

    struct XACPMorphTargetTransform
    {
        uint32_t node_index{0};
        FileQuaternion rotation;
        FileQuaternion scale_rotation;
        FileVector3 position;
        FileVector3 scale;

        void read(binreader::BinaryReader &r)
        {
            node_index = r.read<uint32_t>();
            rotation.read(r);
            scale_rotation.read(r);
            position.read(r);
            scale.read(r);
        }
    };

    struct XACPMorphTarget
    {
        float range_min{0.0f};
        float range_max{1.0f};
        uint32_t lod{0};
        uint32_t num_mesh_deform_deltas{0};
        uint32_t num_transformations{0};
        uint32_t phoneme_sets{0};
        std::string name;
        std::vector<XACPMorphTargetMeshDeltas> morph_target_mesh_deltas;
        std::vector<XACPMorphTargetTransform> morph_target_transform;

        void read(binreader::BinaryReader &r)
        {
            range_min = r.read<float>();
            range_max = r.read<float>();
            lod = r.read<uint32_t>();
            num_mesh_deform_deltas = r.read<uint32_t>();
            num_transformations = r.read<uint32_t>();
            phoneme_sets = r.read<uint32_t>();
            name = detail::read_len_str(r);
            morph_target_mesh_deltas.resize(num_mesh_deform_deltas);
            for (auto &d : morph_target_mesh_deltas)
                d.read(r);
            morph_target_transform.resize(num_transformations);
            for (auto &t : morph_target_transform)
                t.read(r);
        }
    };

    struct XACPMorphTargets
    {
        uint32_t num_morph_targets{0};
        uint32_t lod{0};
        std::vector<XACPMorphTarget> morph_targets;

        void read(binreader::BinaryReader &r)
        {
            num_morph_targets = r.read<uint32_t>();
            lod = r.read<uint32_t>();
            morph_targets.resize(num_morph_targets);
            for (auto &mt : morph_targets)
                mt.read(r);
        }
    };

    // -------------------------------------------------------
    //  FX material parameters
    // -------------------------------------------------------
    struct XACFXIntParameter
    {
        int32_t value{0};
        std::string name;
        void read(binreader::BinaryReader &r)
        {
            value = r.read<int32_t>();
            name = detail::read_len_str(r);
        }
    };

    struct XACFXFloatParameter
    {
        float value{0.0f};
        std::string name;
        void read(binreader::BinaryReader &r)
        {
            value = r.read<float>();
            name = detail::read_len_str(r);
        }
    };

    struct XACFXColorParameter
    {
        FileColor value;
        std::string name;
        void read(binreader::BinaryReader &r)
        {
            value.read(r);
            name = detail::read_len_str(r);
        }
    };

    struct XACFXVector3Parameter
    {
        FileVector3 value;
        std::string name;
        void read(binreader::BinaryReader &r)
        {
            value.read(r);
            name = detail::read_len_str(r);
        }
    };

    struct XACFXBoolParameter
    {
        uint8_t value{0};
        std::string name;
        void read(binreader::BinaryReader &r)
        {
            value = r.read<uint8_t>();
            name = detail::read_len_str(r);
        }
    };

    struct XACFXBitmapParameter
    {
        std::string name;
        std::string value_name;
        void read(binreader::BinaryReader &r)
        {
            name = detail::read_len_str(r);
            value_name = detail::read_len_str(r);
        }
    };

    struct XACFXMaterial
    {
        uint32_t num_int_params{0};
        uint32_t num_float_params{0};
        uint32_t num_color_params{0};
        uint32_t num_bitmap_params{0};
        std::string name;
        std::string effect_file;
        std::string shader_technique;
        std::vector<XACFXIntParameter> int_params;
        std::vector<XACFXFloatParameter> float_params;
        std::vector<XACFXColorParameter> color_params;
        std::vector<XACFXBitmapParameter> bitmap_params;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            num_int_params = r.read<uint32_t>();
            num_float_params = r.read<uint32_t>();
            num_color_params = r.read<uint32_t>();
            num_bitmap_params = r.read<uint32_t>();
            name = read_len_str(r);
            effect_file = read_len_str(r);
            shader_technique = read_len_str(r);
            int_params.resize(num_int_params);
            for (auto &p : int_params)
                p.read(r);
            float_params.resize(num_float_params);
            for (auto &p : float_params)
                p.read(r);
            color_params.resize(num_color_params);
            for (auto &p : color_params)
                p.read(r);
            bitmap_params.resize(num_bitmap_params);
            for (auto &p : bitmap_params)
                p.read(r);
        }
    };

    struct XACFXMaterial2
    {
        uint32_t num_int_params{0};
        uint32_t num_float_params{0};
        uint32_t num_color_params{0};
        uint32_t num_bool_params{0};
        uint32_t num_vector3_params{0};
        uint32_t num_bitmap_params{0};
        std::string name;
        std::string effect_file;
        std::string shader_technique;
        std::vector<XACFXIntParameter> int_params;
        std::vector<XACFXFloatParameter> float_params;
        std::vector<XACFXColorParameter> color_params;
        std::vector<XACFXBoolParameter> bool_params;
        std::vector<XACFXVector3Parameter> vector3_params;
        std::vector<XACFXBitmapParameter> bitmap_params;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            num_int_params = r.read<uint32_t>();
            num_float_params = r.read<uint32_t>();
            num_color_params = r.read<uint32_t>();
            num_bool_params = r.read<uint32_t>();
            num_vector3_params = r.read<uint32_t>();
            num_bitmap_params = r.read<uint32_t>();
            name = read_len_str(r);
            effect_file = read_len_str(r);
            shader_technique = read_len_str(r);
            int_params.resize(num_int_params);
            for (auto &p : int_params)
                p.read(r);
            float_params.resize(num_float_params);
            for (auto &p : float_params)
                p.read(r);
            color_params.resize(num_color_params);
            for (auto &p : color_params)
                p.read(r);
            bool_params.resize(num_bool_params);
            for (auto &p : bool_params)
                p.read(r);
            vector3_params.resize(num_vector3_params);
            for (auto &p : vector3_params)
                p.read(r);
            bitmap_params.resize(num_bitmap_params);
            for (auto &p : bitmap_params)
                p.read(r);
        }
    };

    struct XACFXMaterial3 : XACFXMaterial2
    {
        uint32_t lod{0};

        void read(binreader::BinaryReader &r)
        {
            lod = r.read<uint32_t>();
            XACFXMaterial2::read(r);
        }
    };

    // -------------------------------------------------------
    //  Skinning info structs
    // -------------------------------------------------------
    struct XACSkinInfluence
    {
        float weight{0.0f};
        uint32_t node_number{0};

        void read(binreader::BinaryReader &r)
        {
            weight = r.read<float>();
            node_number = r.read<uint32_t>();
        }
    };

    struct XACSkinInfoPerVertex
    {
        uint8_t num_influences{0};
        std::vector<XACSkinInfluence> influences;

        void read(binreader::BinaryReader &r)
        {
            num_influences = r.read<uint8_t>();
            influences.resize(num_influences);
            for (auto &inf : influences)
                inf.read(r);
        }
    };

    struct XACSkinningInfoTableEntry
    {
        uint32_t start_index{0};
        uint32_t num_elements{0};

        void read(binreader::BinaryReader &r)
        {
            start_index = r.read<uint32_t>();
            num_elements = r.read<uint32_t>();
        }
    };

    struct XACSkinningInfo // version 1
    {
        uint32_t node_index{0};
        uint8_t is_for_collision_mesh{0};
        std::vector<XACSkinInfoPerVertex> skinning_influence;

        void read(binreader::BinaryReader &r, uint32_t num_org_verts)
        {
            node_index = r.read<uint32_t>();
            is_for_collision_mesh = r.read<uint8_t>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            skinning_influence.resize(num_org_verts);
            for (auto &v : skinning_influence)
                v.read(r);
        }
    };

    struct XACSkinningInfo2 // version 2
    {
        uint32_t node_index{0};
        uint32_t num_total_influences{0};
        uint8_t is_for_collision_mesh{0};
        std::vector<XACSkinInfluence> skinning_influence;
        std::vector<XACSkinningInfoTableEntry> skinning_info_table;

        void read(binreader::BinaryReader &r, uint32_t num_org_verts)
        {
            node_index = r.read<uint32_t>();
            num_total_influences = r.read<uint32_t>();
            is_for_collision_mesh = r.read<uint8_t>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            skinning_influence.resize(num_total_influences);
            for (auto &inf : skinning_influence)
                inf.read(r);
            skinning_info_table.resize(num_org_verts);
            for (auto &e : skinning_info_table)
                e.read(r);
        }
    };

    struct XACSkinningInfo3 // version 3
    {
        uint32_t node_index{0};
        uint32_t num_local_bones{0};
        uint32_t num_total_influences{0};
        uint8_t is_for_collision_mesh{0};
        std::vector<XACSkinInfluence> skinning_influence;
        std::vector<XACSkinningInfoTableEntry> skinning_info_table;

        void read(binreader::BinaryReader &r, uint32_t num_org_verts)
        {
            node_index = r.read<uint32_t>();
            num_local_bones = r.read<uint32_t>();
            num_total_influences = r.read<uint32_t>();
            is_for_collision_mesh = r.read<uint8_t>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            skinning_influence.resize(num_total_influences);
            for (auto &inf : skinning_influence)
                inf.read(r);
            skinning_info_table.resize(num_org_verts);
            for (auto &e : skinning_info_table)
                e.read(r);
        }
    };

    struct XACSkinningInfo4 // version 4
    {
        uint32_t node_index{0};
        uint32_t lod{0};
        uint32_t num_local_bones{0};
        uint32_t num_total_influences{0};
        uint8_t is_for_collision_mesh{0};
        std::vector<XACSkinInfluence> skinning_influence;
        std::vector<XACSkinningInfoTableEntry> skinning_info_table;

        void read(binreader::BinaryReader &r, uint32_t num_org_verts)
        {
            node_index = r.read<uint32_t>();
            lod = r.read<uint32_t>();
            num_local_bones = r.read<uint32_t>();
            num_total_influences = r.read<uint32_t>();
            is_for_collision_mesh = r.read<uint8_t>();
            uint8_t pad[3];
            r.read_bytes(pad, 3);
            skinning_influence.resize(num_total_influences);
            for (auto &inf : skinning_influence)
                inf.read(r);
            skinning_info_table.resize(num_org_verts);
            for (auto &e : skinning_info_table)
                e.read(r);
        }
    };

    // -------------------------------------------------------
    //  Misc chunk types
    // -------------------------------------------------------
    struct XACNodeGroup
    {
        uint16_t num_nodes{0};
        uint8_t disabled_on_default{0};
        std::string name;
        std::vector<uint16_t> data;

        void read(binreader::BinaryReader &r)
        {
            num_nodes = r.read<uint16_t>();
            disabled_on_default = r.read<uint8_t>();
            name = detail::read_len_str(r);
            data.resize(num_nodes);
            for (auto &d : data)
                d = r.read<uint16_t>();
        }
    };

    struct XACNodes
    {
        uint32_t num_nodes{0};
        uint32_t num_root_nodes{0};
        std::vector<XACNode4> xac_node;

        void read(binreader::BinaryReader &r)
        {
            num_nodes = r.read<uint32_t>();
            num_root_nodes = r.read<uint32_t>();
            xac_node.resize(num_nodes);
            for (auto &n : xac_node)
                n.read(r);
        }
    };

    struct XACMaterialInfo
    {
        uint32_t num_total_materials{0};
        uint32_t num_standard_materials{0};
        uint32_t num_fx_materials{0};

        void read(binreader::BinaryReader &r)
        {
            num_total_materials = r.read<uint32_t>();
            num_standard_materials = r.read<uint32_t>();
            num_fx_materials = r.read<uint32_t>();
        }
    };

    struct XACMaterialInfo2
    {
        uint32_t lod{0};
        uint32_t num_total_materials{0};
        uint32_t num_standard_materials{0};
        uint32_t num_fx_materials{0};

        void read(binreader::BinaryReader &r)
        {
            lod = r.read<uint32_t>();
            num_total_materials = r.read<uint32_t>();
            num_standard_materials = r.read<uint32_t>();
            num_fx_materials = r.read<uint32_t>();
        }
    };

    struct XACMeshLodLevel
    {
        uint32_t lod_level{0};
        uint32_t size_in_bytes{0};
        std::vector<uint8_t> lod_model_file;

        void read(binreader::BinaryReader &r)
        {
            lod_level = r.read<uint32_t>();
            size_in_bytes = r.read<uint32_t>();
            lod_model_file.resize(size_in_bytes);
            r.read_bytes(lod_model_file.data(), size_in_bytes);
        }
    };

    struct XACNodeMotionSources
    {
        uint32_t num_nodes{0};
        std::vector<uint16_t> node_indices;

        void read(binreader::BinaryReader &r)
        {
            num_nodes = r.read<uint32_t>();
            node_indices.resize(num_nodes);
            for (auto &i : node_indices)
                i = r.read<uint16_t>();
        }
    };

    struct XACAttachmentNodes
    {
        uint32_t num_nodes{0};
        std::vector<uint16_t> attachment_indices;

        void read(binreader::BinaryReader &r)
        {
            num_nodes = r.read<uint32_t>();
            attachment_indices.resize(num_nodes);
            for (auto &i : attachment_indices)
                i = r.read<uint16_t>();
        }
    };

    // -------------------------------------------------------
    //  Chunk data variant  (all possible payload types)
    // -------------------------------------------------------
    using XACChunkData = std::variant<
        // Info
        XACInfo, XACInfo2, XACInfo3, XACInfo4,
        // Nodes
        XACNode, XACNode2, XACNode3, XACNode4, XACNodes,
        // Skinning
        XACSkinningInfo, XACSkinningInfo2, XACSkinningInfo3, XACSkinningInfo4,
        // Standard materials
        XACStandardMaterial, XACStandardMaterial2, XACStandardMaterial3,
        XACStandardMaterialLayer, XACStandardMaterialLayer2,
        // Mesh
        XACMesh, XACMesh2,
        // Limit
        XACLimit,
        // Morph targets
        XACPMorphTarget, XACPMorphTargets,
        // FX materials
        XACFXMaterial, XACFXMaterial2, XACFXMaterial3,
        // Misc
        XACNodeGroup,
        XACMaterialInfo, XACMaterialInfo2,
        XACMeshLodLevel,
        XACNodeMotionSources,
        XACAttachmentNodes>;

    struct XACChunkEntry
    {
        FileChunk chunk;
        XACChunkData chunk_data;
    };

    // -------------------------------------------------------
    //  XACRoot  –  top-level document object
    // -------------------------------------------------------
    class XACRoot
    {
    public:
        XACHeader header;
        std::vector<XACChunkEntry> chunks;

        static XACRoot from_file(const std::filesystem::path &path)
        {
            auto data = read_file_bytes(path);
            return from_bytes(data.data(), data.size());
        }

        static XACRoot from_bytes(const uint8_t *data, std::size_t size)
        {
            std::vector<uint8_t> buf(data, data + size);
            binreader::BinaryReader reader(std::move(buf));
            return from_reader(reader);
        }

        static XACRoot from_bytes(const std::vector<uint8_t> &data)
        {
            binreader::BinaryReader reader(data);
            return from_reader(reader);
        }

        // Collect all texture / material names referenced in this actor.
        std::vector<std::string> get_texture_names() const
        {
            std::vector<std::string> names;
            for (auto &e : chunks)
            {
                if (e.chunk.chunk_id == static_cast<uint32_t>(XACChunkId::StdMaterial))
                {
                    switch (e.chunk.version)
                    {
                    case 1:
                        if (auto *p = std::get_if<XACStandardMaterial>(&e.chunk_data))
                            names.push_back(p->material_name);
                        break;
                    case 2:
                        if (auto *p = std::get_if<XACStandardMaterial2>(&e.chunk_data))
                            names.push_back(p->material_name);
                        break;
                    case 3:
                        if (auto *p = std::get_if<XACStandardMaterial3>(&e.chunk_data))
                            names.push_back(p->material_name);
                        break;
                    default:
                        break;
                    }
                }
                else if (e.chunk.chunk_id == static_cast<uint32_t>(XACChunkId::FXMaterial))
                {
                    auto collect_bitmaps = [&](const auto &mat)
                    {
                        for (auto &bp : mat.bitmap_params)
                            names.push_back(bp.value_name);
                    };
                    switch (e.chunk.version)
                    {
                    case 1:
                        if (auto *p = std::get_if<XACFXMaterial>(&e.chunk_data))
                            collect_bitmaps(*p);
                        break;
                    case 2:
                        if (auto *p = std::get_if<XACFXMaterial2>(&e.chunk_data))
                            collect_bitmaps(*p);
                        break;
                    case 3:
                        if (auto *p = std::get_if<XACFXMaterial3>(&e.chunk_data))
                            collect_bitmaps(*p);
                        break;
                    default:
                        break;
                    }
                }
            }
            return names;
        }

        std::vector<std::string> get_texture_names_with_path(const std::string &texture_path) const
        {
            auto names = get_texture_names();
            for (auto &n : names)
                n = texture_path + n;
            return names;
        }

    private:
        // Tracks last seen num_org_verts for skinning chunk dispatch
        static XACRoot from_reader(binreader::BinaryReader &r)
        {
            XACRoot root;
            root.header.read(r);
            if (!root.header.is_valid())
                throw std::runtime_error("XAC: invalid fourcc (expected 'XAC ')");
            root.chunks = read_chunks(r);
            return root;
        }

        static std::vector<XACChunkEntry> read_chunks(binreader::BinaryReader &r)
        {
            std::vector<XACChunkEntry> result;
            uint32_t last_num_org_verts = 0; // context for skinning chunks

            while (!r.eof())
            {
                FileChunk chunk;
                try
                {
                    chunk.read(r);
                }
                catch (...)
                {
                    break;
                }

                std::size_t start_pos = r.tell();
                std::size_t end_pos = start_pos + chunk.size_in_bytes;

                auto data = parse_chunk(chunk, r, last_num_org_verts);
                if (data)
                {
                    // Update org_vert context from mesh chunks
                    if (auto *m = std::get_if<XACMesh>(&*data))
                        last_num_org_verts = m->num_org_verts;
                    else if (auto *m = std::get_if<XACMesh2>(&*data))
                        last_num_org_verts = m->num_org_verts;

                    result.push_back({chunk, std::move(*data)});
                }
                else
                {
                    std::cerr << "XAC: skipping unknown chunk_id=" << chunk.chunk_id
                              << " version=" << chunk.version << '\n';
                }

                if (r.tell() < end_pos)
                    r.seek(end_pos);
            }
            return result;
        }

        static std::optional<XACChunkData> parse_chunk(const FileChunk &chunk,
                                                       binreader::BinaryReader &r,
                                                       uint32_t num_org_verts)
        {
            auto id = static_cast<XACChunkId>(chunk.chunk_id);

            switch (id)
            {
            case XACChunkId::Info:
                switch (chunk.version)
                {
                case 1:
                {
                    XACInfo v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACInfo2 v;
                    v.read(r);
                    return v;
                }
                case 3:
                {
                    XACInfo3 v;
                    v.read(r);
                    return v;
                }
                case 4:
                {
                    XACInfo4 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::Node:
                switch (chunk.version)
                {
                case 1:
                {
                    XACNode v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACNode2 v;
                    v.read(r);
                    return v;
                }
                case 3:
                {
                    XACNode3 v;
                    v.read(r);
                    return v;
                }
                case 4:
                {
                    XACNode4 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::Nodes:
            {
                XACNodes v;
                v.read(r);
                return v;
            }

            case XACChunkId::Mesh:
                switch (chunk.version)
                {
                case 1:
                {
                    XACMesh v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACMesh2 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::SkinningInfo:
                switch (chunk.version)
                {
                case 1:
                {
                    XACSkinningInfo v;
                    v.read(r, num_org_verts);
                    return v;
                }
                case 2:
                {
                    XACSkinningInfo2 v;
                    v.read(r, num_org_verts);
                    return v;
                }
                case 3:
                {
                    XACSkinningInfo3 v;
                    v.read(r, num_org_verts);
                    return v;
                }
                case 4:
                {
                    XACSkinningInfo4 v;
                    v.read(r, num_org_verts);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::StdMaterial:
                switch (chunk.version)
                {
                case 1:
                {
                    XACStandardMaterial v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACStandardMaterial2 v;
                    v.read(r);
                    return v;
                }
                case 3:
                {
                    XACStandardMaterial3 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::StdMaterialLayer:
                switch (chunk.version)
                {
                case 1:
                {
                    XACStandardMaterialLayer v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACStandardMaterialLayer2 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::FXMaterial:
                switch (chunk.version)
                {
                case 1:
                {
                    XACFXMaterial v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACFXMaterial2 v;
                    v.read(r);
                    return v;
                }
                case 3:
                {
                    XACFXMaterial3 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::Limit:
            {
                XACLimit v;
                v.read(r);
                return v;
            }

            case XACChunkId::StdProgMorphTarget:
            {
                XACPMorphTarget v;
                v.read(r);
                return v;
            }

            case XACChunkId::StdPMorphTargets:
            {
                XACPMorphTargets v;
                v.read(r);
                return v;
            }

            case XACChunkId::NodeGroups:
            {
                XACNodeGroup v;
                v.read(r);
                return v;
            }

            case XACChunkId::MaterialInfo:
                switch (chunk.version)
                {
                case 1:
                {
                    XACMaterialInfo v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XACMaterialInfo2 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }

            case XACChunkId::MeshLodLevels:
            {
                XACMeshLodLevel v;
                v.read(r);
                return v;
            }

            case XACChunkId::NodeMotionSources:
            {
                XACNodeMotionSources v;
                v.read(r);
                return v;
            }

            case XACChunkId::AttachmentNodes:
            {
                XACAttachmentNodes v;
                v.read(r);
                return v;
            }

            default:
                return std::nullopt;
            }
        }

        static std::vector<uint8_t> read_file_bytes(const std::filesystem::path &path)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f)
                throw std::runtime_error("XAC: cannot open file: " + path.string());
            return {std::istreambuf_iterator<char>(f), {}};
        }
    };

} // namespace xac

#endif // XAC_HPP