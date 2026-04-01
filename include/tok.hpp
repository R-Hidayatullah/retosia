#pragma once
#ifndef TOK_HPP
#define TOK_HPP

// ============================================================
//  tok.hpp  –  TOK binary document parser (C++20 port of tok.rs)
// ============================================================
//  The .tok format is a compact binary XML-like tree:
//    1. Null-terminated element names (terminated by empty name)
//    2. Type byte + null-terminated attribute names (terminated by type=0)
//    3. Recursive nodes: element_idx, attr pairs (idx=0 closes), children
//  Dependencies: standard library only
//  Requires:     C++20
// ============================================================

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <ostream>
#include <functional>
#include <memory>
#include <optional>

namespace tok
{

    // -------------------------------------------------------
    //  TokAttrType
    // -------------------------------------------------------
    enum class TokAttrType : uint8_t
    {
        CString = 1,
        SInt32 = 2,
        SInt16 = 3,
        SInt8 = 4,
        UInt32 = 5,
        UInt16 = 6,
        UInt8 = 7,
    };

    inline std::optional<TokAttrType> tok_attr_type_from_u8(uint8_t v)
    {
        if (v >= 1 && v <= 7)
            return static_cast<TokAttrType>(v);
        return std::nullopt;
    }

    // -------------------------------------------------------
    //  TokNode  –  one node in the document tree
    // -------------------------------------------------------
    struct TokNode
    {
        uint8_t element_index{0};
        std::string element_name;
        std::vector<std::pair<std::string, std::string>> attributes;
        std::vector<TokNode> children;
    };

    // -------------------------------------------------------
    //  TokParser
    // -------------------------------------------------------
    class TokParser
    {
    public:
        // Construct from a byte buffer already in memory.
        explicit TokParser(std::vector<uint8_t> data)
            : buf_(std::move(data)), pos_(0) {}

        // Construct by reading from a file path.
        explicit TokParser(const std::string &path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
                throw std::runtime_error("Cannot open tok file: " + path);
            buf_ = std::vector<uint8_t>(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            pos_ = 0;
        }

        // Parse the whole document and return the root node.
        TokNode parse()
        {
            parse_element_names();
            parse_attribute_types();
            auto root = parse_node();
            if (!root)
                throw std::runtime_error("TOK: empty document");
            return std::move(*root);
        }

    private:
        std::vector<uint8_t> buf_;
        std::size_t pos_;
        std::unordered_map<uint8_t, std::string> element_names_;
        std::unordered_map<uint8_t,
                           std::pair<TokAttrType, std::string>>
            attribute_types_;

        // --- Low-level readers ---
        uint8_t read_u8()
        {
            if (pos_ >= buf_.size())
                throw std::runtime_error("TOK: unexpected end of buffer");
            return buf_[pos_++];
        }

        int8_t read_i8() { return static_cast<int8_t>(read_u8()); }

        int16_t read_le_i16()
        {
            uint8_t lo = read_u8(), hi = read_u8();
            return static_cast<int16_t>(lo | (static_cast<uint16_t>(hi) << 8));
        }

        uint16_t read_le_u16()
        {
            uint8_t lo = read_u8(), hi = read_u8();
            return static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
        }

        int32_t read_le_i32()
        {
            uint32_t v = 0;
            for (int i = 0; i < 4; ++i)
                v |= static_cast<uint32_t>(read_u8()) << (i * 8);
            return static_cast<int32_t>(v);
        }

        uint32_t read_le_u32()
        {
            uint32_t v = 0;
            for (int i = 0; i < 4; ++i)
                v |= static_cast<uint32_t>(read_u8()) << (i * 8);
            return v;
        }

        std::string read_cstring()
        {
            std::string s;
            while (pos_ < buf_.size() && buf_[pos_] != 0)
                s.push_back(static_cast<char>(buf_[pos_++]));
            ++pos_; // skip null terminator
            return s;
        }

        // --- Parsing phases ---
        void parse_element_names()
        {
            uint8_t idx = 1;
            for (;;)
            {
                auto s = read_cstring();
                if (s.empty())
                    break;
                element_names_[idx++] = std::move(s);
            }
        }

        void parse_attribute_types()
        {
            uint8_t idx = 1;
            for (;;)
            {
                uint8_t t = read_u8();
                if (t == 0)
                    break;
                auto name = read_cstring();
                TokAttrType attr_type = tok_attr_type_from_u8(t).value_or(TokAttrType::CString);
                attribute_types_[idx++] = {attr_type, std::move(name)};
            }
        }

        std::string read_attr_value(TokAttrType type)
        {
            switch (type)
            {
            case TokAttrType::CString:
                return read_cstring();
            case TokAttrType::SInt8:
                return std::to_string(read_i8());
            case TokAttrType::SInt16:
                return std::to_string(read_le_i16());
            case TokAttrType::SInt32:
                return std::to_string(read_le_i32());
            case TokAttrType::UInt8:
                return std::to_string(read_u8());
            case TokAttrType::UInt16:
                return std::to_string(read_le_u16());
            case TokAttrType::UInt32:
                return std::to_string(read_le_u32());
            }
            return {};
        }

        std::optional<TokNode> parse_node()
        {
            uint8_t elem_idx = read_u8();
            if (elem_idx == 0)
                return std::nullopt;

            TokNode node;
            node.element_index = elem_idx;
            if (auto it = element_names_.find(elem_idx); it != element_names_.end())
                node.element_name = it->second;
            else
                node.element_name = "Unknown" + std::to_string(elem_idx);

            // Read attributes
            for (;;)
            {
                uint8_t attr_idx = read_u8();
                if (attr_idx == 0)
                    break;
                auto it = attribute_types_.find(attr_idx);
                if (it == attribute_types_.end())
                    continue;
                auto [attr_type, attr_name] = it->second;
                std::string value = read_attr_value(attr_type);
                node.attributes.emplace_back(attr_name, std::move(value));
            }

            // Read children
            for (;;)
            {
                auto child = parse_node();
                if (!child)
                    break;
                node.children.push_back(std::move(*child));
            }

            return node;
        }
    };

    // -------------------------------------------------------
    //  Utility: print a TokNode tree to an ostream
    // -------------------------------------------------------
    inline void print_tok_tree(std::ostream &os, const TokNode &node, int depth = 0)
    {
        std::string indent(depth * 2, ' ');
        os << indent << "Element: " << node.element_name << '\n';
        for (auto &[k, v] : node.attributes)
            os << indent << "  Attribute: " << k << " = " << v << '\n';
        for (auto &child : node.children)
            print_tok_tree(os, child, depth + 1);
    }

    // -------------------------------------------------------
    //  SVG export from TOK mesh data
    //  Reads mesh3D/verts and mappingTo2D polygons.
    // -------------------------------------------------------
    namespace detail
    {
        inline const TokNode *find_node(const TokNode &root, std::string_view name)
        {
            auto lower = [](std::string s)
            {
                for (auto &c : s)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return s;
            };
            if (lower(root.element_name) == lower(std::string(name)))
                return &root;
            for (auto &child : root.children)
                if (auto *found = find_node(child, name))
                    return found;
            return nullptr;
        }

        inline std::string lower(const std::string &s)
        {
            std::string r = s;
            for (auto &c : r)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return r;
        }

        inline float attr_f(const TokNode &n, std::string_view key)
        {
            for (auto &[k, v] : n.attributes)
                if (lower(k) == std::string(key))
                {
                    try
                    {
                        return std::stof(v);
                    }
                    catch (...)
                    {
                    }
                }
            return 0.0f;
        }

        inline std::size_t attr_sz(const TokNode &n, std::string_view key)
        {
            for (auto &[k, v] : n.attributes)
                if (lower(k) == std::string(key))
                {
                    try
                    {
                        return static_cast<std::size_t>(std::stoul(v));
                    }
                    catch (...)
                    {
                    }
                }
            return 0;
        }
    } // namespace detail

    inline void export_to_svg(const TokNode &root, std::ostream &out,
                              float width = 500.0f, float height = 500.0f)
    {
        using namespace detail;

        const TokNode *mesh3d = find_node(root, "mesh3d");
        const TokNode *map2d = find_node(root, "mappingto2d");

        if (!mesh3d)
            throw std::runtime_error("TOK SVG: no mesh3D node found");
        if (!map2d)
            throw std::runtime_error("TOK SVG: no mappingTo2D node found");

        // Locate verts child of mesh3d
        const TokNode *verts_node = nullptr;
        for (auto &c : mesh3d->children)
            if (lower(c.element_name) == "verts")
            {
                verts_node = &c;
                break;
            }

        // Build bounding box
        float min_x = 1e30f, max_x = -1e30f;
        float min_y = 1e30f, max_y = -1e30f;
        if (verts_node)
        {
            for (auto &v : verts_node->children)
            {
                float x = attr_f(v, "x"), y = attr_f(v, "y");
                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                min_y = std::min(min_y, y);
                max_y = std::max(max_y, y);
            }
        }

        float range_x = std::max(max_x - min_x, 1.0f);
        float range_y = std::max(max_y - min_y, 1.0f);
        float scale = std::min(width / range_x, height / range_y) * 0.9f;
        float off_x = width / 2.0f - (min_x + max_x) / 2.0f * scale;
        float off_y = height / 2.0f + (min_y + max_y) / 2.0f * scale;

        out << R"(<?xml version="1.0" standalone="no"?>)" << '\n';
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
            << " width=\"" << width << "\" height=\"" << height << "\">\n";

        for (auto &polygon : map2d->children)
        {
            std::string pts;
            for (auto &edge : polygon.children)
            {
                std::size_t si = attr_sz(edge, "startvert");
                if (verts_node && si < verts_node->children.size())
                {
                    auto &v = verts_node->children[si];
                    float x = attr_f(v, "x") * scale + off_x;
                    float y = -attr_f(v, "y") * scale + off_y;
                    pts += std::to_string(x) + ',' + std::to_string(y) + ' ';
                }
            }
            if (!pts.empty())
                out << "<polygon points=\"" << pts
                    << "\" fill=\"#F2BC65\" stroke=\"#F2BC65\" stroke-width=\"1\"/>\n";
        }

        out << "</svg>\n";
    }

} // namespace tok

#endif // TOK_HPP