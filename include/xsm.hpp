#pragma once
#ifndef XSM_HPP
#define XSM_HPP

// ============================================================
//  xsm.hpp  –  Skeletal Motion parser (C++20 port of xsm.rs)
// ============================================================
//  XSM files contain skeletal animation tracks with optional
//  wavelet compression.
//
//  Chunk layout (little-endian):
//    XSMHeader  (8 bytes)
//    Repeated:  FileChunk header (12 bytes) + chunk payload
//
//  Supported chunk IDs:
//    201 = INFO          → XSMInfo / XSMInfo2 / XSMInfo3
//    202 = SUBMOTIONS    → XSMSubMotions / XSMSubMotions2
//    203 = WAVELET_INFO  → XSMWaveletInfo
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

namespace xsm
{

    // -------------------------------------------------------
    // Wavelet types used during compression
    // -------------------------------------------------------
    enum class WaveletType : uint8_t
    {
        Haar = 0,
        D4 = 1,
        CDF97 = 2,
    };

    // -------------------------------------------------------
    // Compressor types for quantized data
    // -------------------------------------------------------
    enum class CompressorType : uint8_t
    {
        Huffman = 0,
        Rice = 1,
    };

    // -------------------------------------------------------
    //  Chunk ID constants
    // -------------------------------------------------------
    enum class XSMChunkId : uint32_t
    {
        SubMotion = 200,
        Info = 201,
        MotionEventTable = 50,
        SubMotions = 202,
        WaveletInfo = 203,
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
    //  Shared math types
    // -------------------------------------------------------
    struct FileVector3
    {
        float x{}, y{}, z{};
        void read(binreader::BinaryReader &r)
        {
            x = r.read<float>();
            y = r.read<float>();
            z = r.read<float>();
        }
    };

    struct FileQuaternion
    {
        float x{}, y{}, z{}, w{};
        void read(binreader::BinaryReader &r)
        {
            x = r.read<float>();
            y = r.read<float>();
            z = r.read<float>();
            w = r.read<float>();
        }
    };

    struct File16BitQuaternion
    {
        int16_t x{}, y{}, z{}, w{};
        void read(binreader::BinaryReader &r)
        {
            x = r.read<int16_t>();
            y = r.read<int16_t>();
            z = r.read<int16_t>();
            w = r.read<int16_t>();
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
    //  XSM file header  (8 bytes)
    // -------------------------------------------------------
    struct XSMHeader
    {
        char fourcc[4]{}; // "XSM "
        uint8_t hi_version{0};
        uint8_t lo_version{0};
        uint8_t endian_type{0};
        uint8_t mul_order{0};

        void read(binreader::BinaryReader &r)
        {
            r.read_bytes(reinterpret_cast<uint8_t *>(fourcc), 4);
            hi_version = r.read<uint8_t>();
            lo_version = r.read<uint8_t>();
            endian_type = r.read<uint8_t>();
            mul_order = r.read<uint8_t>();
        }

        bool is_valid() const { return fourcc[0] == 'X' && fourcc[1] == 'S' && fourcc[2] == 'M'; }
    };

    // -------------------------------------------------------
    //  INFO chunks (versions 1-3)
    // -------------------------------------------------------
    struct XSMInfo
    {
        uint32_t motion_fps{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string motion_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            motion_fps = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            motion_name = read_len_str(r);
        }
    };

    struct XSMInfo2
    {
        float importance_factor{0.0f};
        float max_acceptable_error{0.0f};
        uint32_t motion_fps{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string motion_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            importance_factor = r.read<float>();
            max_acceptable_error = r.read<float>();
            motion_fps = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            motion_name = read_len_str(r);
        }
    };

    struct XSMInfo3
    {
        float importance_factor{0.0f};
        float max_acceptable_error{0.0f};
        uint32_t motion_fps{0};
        uint32_t motion_extraction_mask{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        std::string source_app;
        std::string original_filename;
        std::string compilation_date;
        std::string motion_name;

        void read(binreader::BinaryReader &r)
        {
            using detail::read_len_str;
            importance_factor = r.read<float>();
            max_acceptable_error = r.read<float>();
            motion_fps = r.read<uint32_t>();
            motion_extraction_mask = r.read<uint32_t>();
            exporter_high_version = r.read<uint8_t>();
            exporter_low_version = r.read<uint8_t>();
            r.read<uint16_t>(); // padding
            source_app = read_len_str(r);
            original_filename = read_len_str(r);
            compilation_date = read_len_str(r);
            motion_name = read_len_str(r);
        }
    };

    // -------------------------------------------------------
    //  Keyframe types
    // -------------------------------------------------------
    struct XSMVector3Key
    {
        FileVector3 value;
        float time{0.0f};

        void read(binreader::BinaryReader &r)
        {
            value.read(r);
            time = r.read<float>();
        }
    };

    struct XSMQuaternionKey
    {
        FileQuaternion value;
        float time{0.0f};

        void read(binreader::BinaryReader &r)
        {
            value.read(r);
            time = r.read<float>();
        }
    };

    struct XSM16BitQuaternionKey
    {
        File16BitQuaternion value;
        float time{0.0f};

        void read(binreader::BinaryReader &r)
        {
            value.read(r);
            time = r.read<float>();
        }
    };

    // -------------------------------------------------------
    //  Skeletal sub-motions (three versions)
    // -------------------------------------------------------
    struct XSMSkeletalSubMotion // version 1 – 32-bit quats, no max_error
    {
        FileQuaternion pose_rot;
        FileQuaternion bind_pose_rot;
        FileQuaternion pose_scale_rot;
        FileQuaternion bind_pose_scale_rot;
        FileVector3 pose_pos;
        FileVector3 pose_scale;
        FileVector3 bind_pose_pos;
        FileVector3 bind_pose_scale;
        uint32_t num_pos_keys{0};
        uint32_t num_rot_keys{0};
        uint32_t num_scale_keys{0};
        uint32_t num_scale_rot_keys{0};
        std::string name;

        std::vector<XSMVector3Key> pos_keys;
        std::vector<XSMQuaternionKey> rot_keys;
        std::vector<XSMVector3Key> scale_keys;
        std::vector<XSMQuaternionKey> scale_rot_keys;

        void read(binreader::BinaryReader &r)
        {
            pose_rot.read(r);
            bind_pose_rot.read(r);
            pose_scale_rot.read(r);
            bind_pose_scale_rot.read(r);
            pose_pos.read(r);
            pose_scale.read(r);
            bind_pose_pos.read(r);
            bind_pose_scale.read(r);
            num_pos_keys = r.read<uint32_t>();
            num_rot_keys = r.read<uint32_t>();
            num_scale_keys = r.read<uint32_t>();
            num_scale_rot_keys = r.read<uint32_t>();
            name = detail::read_len_str(r);

            pos_keys.resize(num_pos_keys);
            for (auto &k : pos_keys)
                k.read(r);
            rot_keys.resize(num_rot_keys);
            for (auto &k : rot_keys)
                k.read(r);
            scale_keys.resize(num_scale_keys);
            for (auto &k : scale_keys)
                k.read(r);
            scale_rot_keys.resize(num_scale_rot_keys);
            for (auto &k : scale_rot_keys)
                k.read(r);
        }
    };

    struct XSMSkeletalSubMotion2 // version 2 – 32-bit quats + max_error
    {
        FileQuaternion pose_rot;
        FileQuaternion bind_pose_rot;
        FileQuaternion pose_scale_rot;
        FileQuaternion bind_pose_scale_rot;
        FileVector3 pose_pos;
        FileVector3 pose_scale;
        FileVector3 bind_pose_pos;
        FileVector3 bind_pose_scale;
        uint32_t num_pos_keys{0};
        uint32_t num_rot_keys{0};
        uint32_t num_scale_keys{0};
        uint32_t num_scale_rot_keys{0};
        float max_error{0.0f};
        std::string name;

        std::vector<XSMVector3Key> pos_keys;
        std::vector<XSMQuaternionKey> rot_keys;
        std::vector<XSMVector3Key> scale_keys;
        std::vector<XSMQuaternionKey> scale_rot_keys;

        void read(binreader::BinaryReader &r)
        {
            pose_rot.read(r);
            bind_pose_rot.read(r);
            pose_scale_rot.read(r);
            bind_pose_scale_rot.read(r);
            pose_pos.read(r);
            pose_scale.read(r);
            bind_pose_pos.read(r);
            bind_pose_scale.read(r);
            num_pos_keys = r.read<uint32_t>();
            num_rot_keys = r.read<uint32_t>();
            num_scale_keys = r.read<uint32_t>();
            num_scale_rot_keys = r.read<uint32_t>();
            max_error = r.read<float>();
            name = detail::read_len_str(r);

            pos_keys.resize(num_pos_keys);
            for (auto &k : pos_keys)
                k.read(r);
            rot_keys.resize(num_rot_keys);
            for (auto &k : rot_keys)
                k.read(r);
            scale_keys.resize(num_scale_keys);
            for (auto &k : scale_keys)
                k.read(r);
            scale_rot_keys.resize(num_scale_rot_keys);
            for (auto &k : scale_rot_keys)
                k.read(r);
        }
    };

    struct XSMSkeletalSubMotion3 // version 3 – 16-bit compressed quats
    {
        File16BitQuaternion pose_rot;
        File16BitQuaternion bind_pose_rot;
        File16BitQuaternion pose_scale_rot;
        File16BitQuaternion bind_pose_scale_rot;
        FileVector3 pose_pos;
        FileVector3 pose_scale;
        FileVector3 bind_pose_pos;
        FileVector3 bind_pose_scale;
        uint32_t num_pos_keys{0};
        uint32_t num_rot_keys{0};
        uint32_t num_scale_keys{0};
        uint32_t num_scale_rot_keys{0};
        float max_error{0.0f};
        std::string name;

        std::vector<XSMVector3Key> pos_keys;
        std::vector<XSM16BitQuaternionKey> rot_keys;
        std::vector<XSMVector3Key> scale_keys;
        std::vector<XSM16BitQuaternionKey> scale_rot_keys;

        void read(binreader::BinaryReader &r)
        {
            pose_rot.read(r);
            bind_pose_rot.read(r);
            pose_scale_rot.read(r);
            bind_pose_scale_rot.read(r);
            pose_pos.read(r);
            pose_scale.read(r);
            bind_pose_pos.read(r);
            bind_pose_scale.read(r);
            num_pos_keys = r.read<uint32_t>();
            num_rot_keys = r.read<uint32_t>();
            num_scale_keys = r.read<uint32_t>();
            num_scale_rot_keys = r.read<uint32_t>();
            max_error = r.read<float>();
            name = detail::read_len_str(r);

            pos_keys.resize(num_pos_keys);
            for (auto &k : pos_keys)
                k.read(r);
            rot_keys.resize(num_rot_keys);
            for (auto &k : rot_keys)
                k.read(r);
            scale_keys.resize(num_scale_keys);
            for (auto &k : scale_keys)
                k.read(r);
            scale_rot_keys.resize(num_scale_rot_keys);
            for (auto &k : scale_rot_keys)
                k.read(r);
        }
    };

    // -------------------------------------------------------
    //  SUBMOTIONS chunk (version 1 → v2 sub-motions, version 2 → v3)
    // -------------------------------------------------------
    struct XSMSubMotions
    {
        uint32_t num_sub_motions{0};
        std::vector<XSMSkeletalSubMotion2> sub_motions; // v1 chunk uses SM v2 structs

        void read(binreader::BinaryReader &r)
        {
            num_sub_motions = r.read<uint32_t>();
            sub_motions.resize(num_sub_motions);
            for (auto &sm : sub_motions)
                sm.read(r);
        }
    };

    struct XSMSubMotions2
    {
        uint32_t num_sub_motions{0};
        std::vector<XSMSkeletalSubMotion3> sub_motions; // v2 chunk uses SM v3 structs

        void read(binreader::BinaryReader &r)
        {
            num_sub_motions = r.read<uint32_t>();
            sub_motions.resize(num_sub_motions);
            for (auto &sm : sub_motions)
                sm.read(r);
        }
    };

    // -------------------------------------------------------
    //  Wavelet types
    // -------------------------------------------------------
    struct XSMWaveletMapping
    {
        uint16_t pos_index{0};
        uint16_t rot_index{0};
        uint16_t scale_rot_index{0};
        uint16_t scale_index{0};

        void read(binreader::BinaryReader &r)
        {
            pos_index = r.read<uint16_t>();
            rot_index = r.read<uint16_t>();
            scale_rot_index = r.read<uint16_t>();
            scale_index = r.read<uint16_t>();
        }
    };

    struct XSMWaveletSkeletalSubMotion
    {
        File16BitQuaternion pose_rot;
        File16BitQuaternion bind_pose_rot;
        File16BitQuaternion pose_scale_rot;
        File16BitQuaternion bind_pose_scale_rot;
        FileVector3 pose_pos;
        FileVector3 pose_scale;
        FileVector3 bind_pose_pos;
        FileVector3 bind_pose_scale;
        float max_error{0.0f};
        std::string name;

        void read(binreader::BinaryReader &r)
        {
            pose_rot.read(r);
            bind_pose_rot.read(r);
            pose_scale_rot.read(r);
            bind_pose_scale_rot.read(r);
            pose_pos.read(r);
            pose_scale.read(r);
            bind_pose_pos.read(r);
            bind_pose_scale.read(r);
            max_error = r.read<float>();
            name = detail::read_len_str(r);
        }
    };

    struct XSMWaveletChunk
    {
        float rot_quant_scale{0.0f};
        float pos_quant_scale{0.0f};
        float scale_quant_scale{0.0f};
        float start_time{0.0f};
        uint32_t compressed_rot_num_bytes{0};
        uint32_t compressed_pos_num_bytes{0};
        uint32_t compressed_scale_num_bytes{0};
        uint32_t compressed_pos_num_bits{0};
        uint32_t compressed_rot_num_bits{0};
        uint32_t compressed_scale_num_bits{0};
        std::vector<uint8_t> compressed_rot_data;
        std::vector<uint8_t> compressed_pos_data;
        std::vector<uint8_t> compressed_scale_data;

        void read(binreader::BinaryReader &r)
        {
            rot_quant_scale = r.read<float>();
            pos_quant_scale = r.read<float>();
            scale_quant_scale = r.read<float>();
            start_time = r.read<float>();
            compressed_rot_num_bytes = r.read<uint32_t>();
            compressed_pos_num_bytes = r.read<uint32_t>();
            compressed_scale_num_bytes = r.read<uint32_t>();
            compressed_pos_num_bits = r.read<uint32_t>();
            compressed_rot_num_bits = r.read<uint32_t>();
            compressed_scale_num_bits = r.read<uint32_t>();

            compressed_rot_data.resize(compressed_rot_num_bytes);
            r.read_bytes(compressed_rot_data.data(), compressed_rot_num_bytes);
            compressed_pos_data.resize(compressed_pos_num_bytes);
            r.read_bytes(compressed_pos_data.data(), compressed_pos_num_bytes);
            compressed_scale_data.resize(compressed_scale_num_bytes);
            r.read_bytes(compressed_scale_data.data(), compressed_scale_num_bytes);
        }
    };

    struct XSMWaveletInfo
    {
        uint32_t num_chunks{0};
        uint32_t samples_per_chunk{0};
        uint32_t decompressed_rot_num_bytes{0};
        uint32_t decompressed_pos_num_bytes{0};
        uint32_t decompressed_scale_num_bytes{0};
        uint32_t num_rot_tracks{0};
        uint32_t num_scale_rot_tracks{0};
        uint32_t num_scale_tracks{0};
        uint32_t num_pos_tracks{0};
        uint32_t chunk_overhead{0};
        uint32_t compressed_size{0};
        uint32_t optimized_size{0};
        uint32_t uncompressed_size{0};
        uint32_t scale_rot_offset{0};
        uint32_t num_sub_motions{0};
        float pos_quant_factor{0.0f};
        float rot_quant_factor{0.0f};
        float scale_quant_factor{0.0f};
        float sample_spacing{0.0f};
        float seconds_per_chunk{0.0f};
        float max_time{0.0f};
        uint8_t wavelet_id{0};
        uint8_t compressor_id{0};
        std::vector<XSMWaveletMapping> mappings;
        std::vector<XSMWaveletSkeletalSubMotion> sub_motions;
        std::vector<XSMWaveletChunk> chunks;

        void read(binreader::BinaryReader &r)
        {
            num_chunks = r.read<uint32_t>();
            samples_per_chunk = r.read<uint32_t>();
            decompressed_rot_num_bytes = r.read<uint32_t>();
            decompressed_pos_num_bytes = r.read<uint32_t>();
            decompressed_scale_num_bytes = r.read<uint32_t>();
            num_rot_tracks = r.read<uint32_t>();
            num_scale_rot_tracks = r.read<uint32_t>();
            num_scale_tracks = r.read<uint32_t>();
            num_pos_tracks = r.read<uint32_t>();
            chunk_overhead = r.read<uint32_t>();
            compressed_size = r.read<uint32_t>();
            optimized_size = r.read<uint32_t>();
            uncompressed_size = r.read<uint32_t>();
            scale_rot_offset = r.read<uint32_t>();
            num_sub_motions = r.read<uint32_t>();
            pos_quant_factor = r.read<float>();
            rot_quant_factor = r.read<float>();
            scale_quant_factor = r.read<float>();
            sample_spacing = r.read<float>();
            seconds_per_chunk = r.read<float>();
            max_time = r.read<float>();
            wavelet_id = r.read<uint8_t>();
            compressor_id = r.read<uint8_t>();
            r.read<uint16_t>(); // padding

            mappings.resize(num_sub_motions);
            for (auto &m : mappings)
                m.read(r);
            sub_motions.resize(num_sub_motions);
            for (auto &sm : sub_motions)
                sm.read(r);
            chunks.resize(num_chunks);
            for (auto &c : chunks)
                c.read(r);
        }
    };

    // -------------------------------------------------------
    //  Chunk data variant
    // -------------------------------------------------------
    using XSMChunkData = std::variant<
        XSMInfo, XSMInfo2, XSMInfo3,
        XSMSubMotions, XSMSubMotions2,
        XSMWaveletInfo>;

    struct XSMChunkEntry
    {
        FileChunk chunk;
        XSMChunkData chunk_data;
    };

    // -------------------------------------------------------
    //  XSMRoot  –  top-level document object
    // -------------------------------------------------------
    class XSMRoot
    {
    public:
        XSMHeader header;
        std::vector<XSMChunkEntry> chunks;

        static XSMRoot from_file(const std::filesystem::path &path)
        {
            auto data = read_file_bytes(path);
            return from_bytes(data.data(), data.size());
        }

        static XSMRoot from_bytes(const uint8_t *data, std::size_t size)
        {
            std::vector<uint8_t> buf(data, data + size);
            binreader::BinaryReader reader(std::move(buf));
            return from_reader(reader);
        }

        static XSMRoot from_bytes(const std::vector<uint8_t> &data)
        {
            binreader::BinaryReader reader(data);
            return from_reader(reader);
        }

    private:
        static XSMRoot from_reader(binreader::BinaryReader &r)
        {
            XSMRoot root;
            root.header.read(r);
            if (!root.header.is_valid())
                throw std::runtime_error("XSM: invalid fourcc (expected 'XSM ')");
            root.chunks = read_chunks(r);
            return root;
        }

        static std::vector<XSMChunkEntry> read_chunks(binreader::BinaryReader &r)
        {
            std::vector<XSMChunkEntry> result;
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

                auto data = parse_chunk(chunk, r);
                if (data)
                    result.push_back({chunk, std::move(*data)});
                else
                    std::cerr << "XSM: skipping unknown chunk_id=" << chunk.chunk_id
                              << " version=" << chunk.version << '\n';

                if (r.tell() < end_pos)
                    r.seek(end_pos);
            }
            return result;
        }

        static std::optional<XSMChunkData> parse_chunk(const FileChunk &chunk,
                                                       binreader::BinaryReader &r)
        {
            auto id = static_cast<XSMChunkId>(chunk.chunk_id);
            switch (id)
            {
            case XSMChunkId::Info:
                switch (chunk.version)
                {
                case 1:
                {
                    XSMInfo v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XSMInfo2 v;
                    v.read(r);
                    return v;
                }
                case 3:
                {
                    XSMInfo3 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }
            case XSMChunkId::SubMotions:
                switch (chunk.version)
                {
                case 1:
                {
                    XSMSubMotions v;
                    v.read(r);
                    return v;
                }
                case 2:
                {
                    XSMSubMotions2 v;
                    v.read(r);
                    return v;
                }
                default:
                    return std::nullopt;
                }
            case XSMChunkId::WaveletInfo:
            {
                XSMWaveletInfo v;
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
                throw std::runtime_error("XSM: cannot open file: " + path.string());
            return {std::istreambuf_iterator<char>(f), {}};
        }
    };

} // namespace xsm

#endif // XSM_HPP