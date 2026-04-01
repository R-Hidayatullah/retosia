#pragma once
#ifndef XPM_HPP
#define XPM_HPP

// ============================================================
//  xpm.hpp  –  Progressive Morph Motion parser (C++20 port of xpm.rs)
// ============================================================
//  XPM files contain facial / morph-target animations, including
//  phoneme sets for speech animation.
//
//  Chunk layout (little-endian):
//    XPMHeader  (8 bytes)
//    Repeated:  FileChunk header (12 bytes) + chunk payload
//
//  Supported chunk IDs:
//    101 = INFO       → XPMInfo
//    102 = SUBMOTIONS → XPMSubMotions
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

namespace xpm
{

    // -------------------------------------------------------
    //  Chunk ID constants
    // -------------------------------------------------------
    enum class XPMChunkId : uint32_t
    {
        SubMotion = 100,
        Info = 101,
        MotionEventTable = 50,
        SubMotions = 102,
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
    //  File chunk header  (shared with XAC / XSM)
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
    //  XPM file header  (8 bytes)
    // -------------------------------------------------------
    struct XPMHeader
    {
        char fourcc[4]{}; // must be "XPM "
        uint8_t hi_version{0};
        uint8_t lo_version{0};
        uint8_t endian_type{0}; // 0 = little, 1 = big
        uint8_t mul_order{0};

        void read(binreader::BinaryReader &r)
        {
            r.read_bytes(reinterpret_cast<uint8_t *>(fourcc), 4);
            hi_version = r.read<uint8_t>();
            lo_version = r.read<uint8_t>();
            endian_type = r.read<uint8_t>();
            mul_order = r.read<uint8_t>();
        }

        bool is_valid() const { return fourcc[0] == 'X' && fourcc[1] == 'P' && fourcc[2] == 'M'; }
    };

    // -------------------------------------------------------
    //  INFO chunk  (chunk_id = 101)
    // -------------------------------------------------------
    struct XPMInfo
    {
        uint32_t motion_fps{0};
        uint8_t exporter_high_version{0};
        uint8_t exporter_low_version{0};
        // 2-byte padding
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

    // -------------------------------------------------------
    //  Compressed 16-bit keyframe
    // -------------------------------------------------------
    struct XPMUnsignedShortKey
    {
        float time{0.0f};
        uint16_t value{0};
        // 2-byte padding

        void read(binreader::BinaryReader &r)
        {
            time = r.read<float>();
            value = r.read<uint16_t>();
            r.read<uint16_t>(); // padding
        }
    };

    // -------------------------------------------------------
    //  Float keyframe
    // -------------------------------------------------------
    struct XPMFloatKey
    {
        float time{0.0f};
        float value{0.0f};

        void read(binreader::BinaryReader &r)
        {
            time = r.read<float>();
            value = r.read<float>();
        }
    };

    // -------------------------------------------------------
    //  Progressive morph sub-motion
    // -------------------------------------------------------
    struct XPMProgressiveSubMotion
    {
        float pose_weight{0.0f};
        float min_weight{0.0f};
        float max_weight{0.0f};
        uint32_t phoneme_set{0};
        uint32_t num_keys{0};
        std::string name;
        std::vector<XPMUnsignedShortKey> xpm_key;

        void read(binreader::BinaryReader &r)
        {
            pose_weight = r.read<float>();
            min_weight = r.read<float>();
            max_weight = r.read<float>();
            phoneme_set = r.read<uint32_t>();
            num_keys = r.read<uint32_t>();
            name = detail::read_len_str(r);

            xpm_key.resize(num_keys);
            for (auto &k : xpm_key)
                k.read(r);
        }
    };

    // -------------------------------------------------------
    //  SUBMOTIONS chunk  (chunk_id = 102)
    // -------------------------------------------------------
    struct XPMSubMotions
    {
        uint32_t num_sub_motions{0};
        std::vector<XPMProgressiveSubMotion> progressive_sub_motions;

        void read(binreader::BinaryReader &r)
        {
            num_sub_motions = r.read<uint32_t>();
            progressive_sub_motions.resize(num_sub_motions);
            for (auto &sm : progressive_sub_motions)
                sm.read(r);
        }
    };

    // -------------------------------------------------------
    //  Chunk data variant
    // -------------------------------------------------------
    using XPMChunkData = std::variant<XPMInfo, XPMSubMotions>;

    struct XPMChunkEntry
    {
        FileChunk chunk;
        XPMChunkData chunk_data;
    };

    // -------------------------------------------------------
    //  XPMRoot  –  top-level document object
    // -------------------------------------------------------
    class XPMRoot
    {
    public:
        XPMHeader header;
        std::vector<XPMChunkEntry> chunks;

        // --- Factory methods ---
        static XPMRoot from_file(const std::filesystem::path &path)
        {
            auto data = read_file_bytes(path);
            return from_bytes(data.data(), data.size());
        }

        static XPMRoot from_bytes(const uint8_t *data, std::size_t size)
        {
            std::vector<uint8_t> buf(data, data + size);
            binreader::BinaryReader reader(std::move(buf));
            return from_reader(reader);
        }

        static XPMRoot from_bytes(const std::vector<uint8_t> &data)
        {
            binreader::BinaryReader reader(data);
            return from_reader(reader);
        }

        // --- Accessors ---
        const XPMInfo *get_info() const
        {
            for (auto &e : chunks)
                if (auto *p = std::get_if<XPMInfo>(&e.chunk_data))
                    return p;
            return nullptr;
        }

        const XPMSubMotions *get_sub_motions() const
        {
            for (auto &e : chunks)
                if (auto *p = std::get_if<XPMSubMotions>(&e.chunk_data))
                    return p;
            return nullptr;
        }

    private:
        static XPMRoot from_reader(binreader::BinaryReader &r)
        {
            XPMRoot root;
            root.header.read(r);
            if (!root.header.is_valid())
                throw std::runtime_error("XPM: invalid fourcc");
            root.chunks = read_chunks(r);
            return root;
        }

        static std::vector<XPMChunkEntry> read_chunks(binreader::BinaryReader &r)
        {
            std::vector<XPMChunkEntry> result;
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

                std::optional<XPMChunkData> data = parse_chunk(chunk, r);
                if (!data)
                {
                    // Unknown chunk – skip it
                    std::cerr << "XPM: skipping unknown chunk_id=" << chunk.chunk_id
                              << " version=" << chunk.version << '\n';
                }
                else
                {
                    result.push_back({chunk, std::move(*data)});
                }

                // Always advance to the declared end of the chunk
                if (r.tell() < end_pos)
                    r.seek(end_pos);
            }
            return result;
        }

        static std::optional<XPMChunkData> parse_chunk(const FileChunk &chunk,
                                                       binreader::BinaryReader &r)
        {
            auto id = static_cast<XPMChunkId>(chunk.chunk_id);
            if (id == XPMChunkId::Info)
            {
                XPMInfo info;
                info.read(r);
                return info;
            }
            if (id == XPMChunkId::SubMotions)
            {
                XPMSubMotions sm;
                sm.read(r);
                return sm;
            }
            return std::nullopt;
        }

        static std::vector<uint8_t> read_file_bytes(const std::filesystem::path &path)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f)
                throw std::runtime_error("XPM: cannot open file: " + path.string());
            return {std::istreambuf_iterator<char>(f), {}};
        }
    };

} // namespace xpm

#endif // XPM_HPP