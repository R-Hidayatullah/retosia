#if !defined(WORLD_PARSER_HPP)
#define WORLD_PARSER_HPP
#pragma once
#include <tinyxml2.h>
#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <sstream>
#include <iostream>

namespace world_parser
{

    // ------------------- Helper Types -------------------
    struct Pos
    {
        std::string pos;
    };

    struct LightMap
    {
        std::string file;
        std::optional<std::string> length;
        std::optional<std::string> offset;
        std::optional<std::string> size;
    };

    // ------------------- Directory Types -------------------
    struct ModelDir
    {
        std::string ipf_name, path;
    };
    struct TexDir
    {
        std::string ipf_name, path;
    };
    struct SubTexDir
    {
        std::string ipf_name, path;
    };
    struct AnimationDir
    {
        std::string ipf_name, path;
    };
    struct ShaTexDir
    {
        std::string ipf_name, path;
    };

    // ------------------- Model -------------------
    struct Model
    {
        std::string file;
        std::string model;
        std::optional<std::string> shadow_map;
        std::optional<std::string> pos;
        std::optional<std::string> rot;
        std::optional<std::string> scale;
    };

    // ------------------- World -------------------
    class World
    {
    public:
        std::vector<ModelDir> model_dirs;
        std::vector<TexDir> tex_dirs;
        std::vector<SubTexDir> sub_tex_dirs;
        std::vector<AnimationDir> animation_dirs;
        std::vector<ShaTexDir> sha_tex_dirs;
        std::vector<LightMap> light_maps;
        std::optional<Pos> stand_on_pos;
        std::vector<Model> models;

        // ------------------- Parsing -------------------
        static World from_file(const std::string &path)
        {
            std::ifstream file(path);
            if (!file.is_open())
                throw std::runtime_error("Failed to open file: " + path);
            std::stringstream buffer;
            buffer << file.rdbuf();
            return from_string(buffer.str());
        }

        static World from_string(const std::string &xml_content)
        {
            World world;
            tinyxml2::XMLDocument doc;
            if (doc.Parse(xml_content.c_str()) != tinyxml2::XML_SUCCESS)
            {
                throw std::runtime_error("XML parse error");
            }

            auto root = doc.FirstChildElement("World");
            if (!root)
                throw std::runtime_error("Missing <World> root element");

            // --- Parse Directories ---
            parse_directories<ModelDir>(root, "ModelDir", world.model_dirs);
            parse_directories<TexDir>(root, "TexDir", world.tex_dirs);
            parse_directories<SubTexDir>(root, "SubTexDir", world.sub_tex_dirs);
            parse_directories<AnimationDir>(root, "AnimationDir", world.animation_dirs);
            parse_directories<ShaTexDir>(root, "ShaTexDir", world.sha_tex_dirs);

            // --- Parse LightMaps ---
            for (auto lm = root->FirstChildElement("LightMap"); lm; lm = lm->NextSiblingElement("LightMap"))
            {
                LightMap l;
                l.file = lm->Attribute("File") ? lm->Attribute("File") : "";
                l.length = lm->Attribute("Length") ? std::optional<std::string>{lm->Attribute("Length")} : std::nullopt;
                l.offset = lm->Attribute("Offset") ? std::optional<std::string>{lm->Attribute("Offset")} : std::nullopt;
                l.size = lm->Attribute("Size") ? std::optional<std::string>{lm->Attribute("Size")} : std::nullopt;
                world.light_maps.push_back(l);
            }

            // --- Parse StandOnPos ---
            if (auto sop = root->FirstChildElement("StandOnPos"))
            {
                Pos p;
                p.pos = sop->Attribute("pos") ? sop->Attribute("pos") : "";
                world.stand_on_pos = p;
            }

            // --- Parse Models ---
            for (auto m = root->FirstChildElement("Model"); m; m = m->NextSiblingElement("Model"))
            {
                Model mod;
                mod.file = m->Attribute("File") ? m->Attribute("File") : "";
                mod.model = m->Attribute("Model") ? m->Attribute("Model") : "";
                mod.shadow_map = m->Attribute("ShadowMap") ? std::optional<std::string>{m->Attribute("ShadowMap")} : std::nullopt;
                mod.pos = m->Attribute("pos") ? std::optional<std::string>{m->Attribute("pos")} : std::nullopt;
                mod.rot = m->Attribute("rot") ? std::optional<std::string>{m->Attribute("rot")} : std::nullopt;
                mod.scale = m->Attribute("scale") ? std::optional<std::string>{m->Attribute("scale")} : std::nullopt;
                world.models.push_back(mod);
            }

            return world;
        }

    private:
        // Generic parser for directory-like nodes
        template <typename DirType>
        static void parse_directories(tinyxml2::XMLElement *root, const char *name, std::vector<DirType> &out)
        {
            for (auto elem = root->FirstChildElement(name); elem; elem = elem->NextSiblingElement(name))
            {
                DirType d;
                d.ipf_name = elem->Attribute("IpfName") ? elem->Attribute("IpfName") : "";
                d.path = elem->Attribute("Path") ? elem->Attribute("Path") : "";
                out.push_back(d);
            }
        }
    };

} // namespace world_parser

#endif // WORLD_PARSER_HPP
