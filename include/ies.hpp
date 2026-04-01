#if !defined(IES_HPP)
#define IES_HPP
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include "binary_reader.hpp" // Include your previous BinaryReader with BinaryReadError

namespace ies
{

    constexpr uint8_t XOR_KEY = 1;

    // Utility: XOR decrypt bytes to string
    inline std::string decrypt_bytes_to_string(const std::vector<uint8_t> &bytes)
    {
        std::string result;
        result.reserve(bytes.size());
        for (auto b : bytes)
            result.push_back(static_cast<char>(b ^ XOR_KEY));
        // Trim non-graphic trailing characters
        result.erase(std::find_if(result.rbegin(), result.rend(),
                                  [](char c)
                                  { return std::isgraph(c) || std::isspace(c); })
                         .base(),
                     result.end());
        return result;
    }

    // Trim padding from a fixed-size byte array
    inline std::string trim_padding(const std::vector<uint8_t> &bytes)
    {
        std::string result(bytes.begin(), bytes.end());
        result.erase(std::find_if(result.rbegin(), result.rend(),
                                  [](char c)
                                  { return std::isgraph(c) || std::isspace(c); })
                         .base(),
                     result.end());
        return result;
    }

    // IES structures
    struct IESColumn
    {
        std::string column;
        std::string name;
        uint16_t type_data;
        uint16_t access_data;
        uint16_t sync_data;
        uint16_t decl_idx;

        void read(binreader::BinaryReader &reader)
        {
            std::vector<uint8_t> buf(64);
            reader.read_bytes(buf.data(), buf.size());
            column = decrypt_bytes_to_string(buf);

            reader.read_bytes(buf.data(), buf.size());
            name = decrypt_bytes_to_string(buf);

            type_data = reader.read<uint16_t>();
            access_data = reader.read<uint16_t>();
            sync_data = reader.read<uint16_t>();
            decl_idx = reader.read<uint16_t>();
        }
    };

    struct IESRowText
    {
        std::string text_data;

        void read(binreader::BinaryReader &reader)
        {
            uint16_t length = reader.read<uint16_t>();
            if (length == 0)
            {
                text_data.clear();
                return;
            }

            std::vector<uint8_t> buf(length);
            reader.read_bytes(buf.data(), buf.size());
            text_data = decrypt_bytes_to_string(buf);
        }
    };

    struct IESRowFloat
    {
        float float_data;

        void read(binreader::BinaryReader &reader)
        {
            float_data = reader.read<float>();
        }
    };

    struct IESColumnData
    {
        int32_t index_data;
        IESRowText row_text;
        std::vector<IESRowFloat> floats;
        std::vector<IESRowText> texts;
        std::vector<int8_t> padding;

        void read(binreader::BinaryReader &reader, uint16_t num_column_number, uint16_t num_column_string)
        {
            index_data = reader.read<int32_t>();
            row_text.read(reader);

            floats.resize(num_column_number);
            for (auto &f : floats)
                f.read(reader);

            texts.resize(num_column_string);
            for (auto &t : texts)
                t.read(reader);

            padding.resize(num_column_string);
            for (auto &p : padding)
                p = reader.read<int8_t>();
        }
    };

    struct IESHeader
    {
        std::string idspace;
        std::string keyspace;
        uint16_t version;
        uint16_t padding;
        uint32_t info_size;
        uint32_t data_size;
        uint32_t total_size;
        uint8_t use_class_id;
        uint8_t padding2;
        uint16_t num_field;
        uint16_t num_column;
        uint16_t num_column_number;
        uint16_t num_column_string;
        uint16_t padding3;

        void read(binreader::BinaryReader &reader)
        {
            std::vector<uint8_t> buf(64);

            reader.read_bytes(buf.data(), buf.size());
            idspace = trim_padding(buf);

            reader.read_bytes(buf.data(), buf.size());
            keyspace = trim_padding(buf);

            version = reader.read<uint16_t>();
            padding = reader.read<uint16_t>();
            info_size = reader.read<uint32_t>();
            data_size = reader.read<uint32_t>();
            total_size = reader.read<uint32_t>();
            use_class_id = reader.read<uint8_t>();
            padding2 = reader.read<uint8_t>();
            num_field = reader.read<uint16_t>();
            num_column = reader.read<uint16_t>();
            num_column_number = reader.read<uint16_t>();
            num_column_string = reader.read<uint16_t>();
            padding3 = reader.read<uint16_t>();
        }
    };

    struct IESRoot
    {
        IESHeader header;
        std::vector<IESColumn> columns;
        std::vector<IESColumnData> data;

        // Read from file
        static IESRoot from_file(const std::string &path)
        {
            binreader::BinaryReader reader(path);
            return from_reader(reader);
        }

        // Read from memory buffer
        static IESRoot from_bytes(const std::vector<uint8_t> &bytes)
        {
            binreader::BinaryReader reader(bytes);
            return from_reader(reader);
        }

        static IESRoot from_reader(binreader::BinaryReader &reader)
        {
            IESRoot root;
            root.header.read(reader);

            root.columns.resize(root.header.num_column);
            for (auto &col : root.columns)
                col.read(reader);

            root.data.resize(root.header.num_field);
            for (auto &row : root.data)
                row.read(reader, root.header.num_column_number, root.header.num_column_string);

            return root;
        }

        std::unordered_map<std::string, std::string> extract_mesh_path_map() const
        {
            // Sort columns by decl_idx then type_data
            std::vector<std::pair<size_t, const IESColumn *>> sorted;
            for (size_t i = 0; i < columns.size(); ++i)
                sorted.emplace_back(i, &columns[i]);
            std::sort(sorted.begin(), sorted.end(),
                      [](auto &a, auto &b)
                      {
                          return a.second->decl_idx < b.second->decl_idx ||
                                 (a.second->decl_idx == b.second->decl_idx && a.second->type_data < b.second->type_data);
                      });

            // Find indices
            int mesh_idx = -1, path_idx = -1;
            for (size_t i = 0; i < sorted.size(); ++i)
            {
                if (sorted[i].second->column == "Mesh")
                    mesh_idx = int(i);
                if (sorted[i].second->column == "Path")
                    path_idx = int(i);
            }
            if (mesh_idx < 0 || path_idx < 0)
                return {};

            std::unordered_map<std::string, std::string> map;
            for (const auto &row : data)
            {
                std::string mesh = (mesh_idx < int(row.texts.size())) ? row.texts[mesh_idx].text_data : "";
                std::string path = (path_idx < int(row.texts.size())) ? row.texts[path_idx].text_data : "";
                if (!mesh.empty() && !path.empty())
                {
                    std::transform(mesh.begin(), mesh.end(), mesh.begin(), ::tolower);
                    std::replace(path.begin(), path.end(), '\\', '/');
                    map[mesh] = path;
                }
            }
            return map;
        }
    };

} // namespace ies

#endif // IES_HPP
