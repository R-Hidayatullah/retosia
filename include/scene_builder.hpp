#if !defined(SCENE_BUILDER_HPP)
#define SCENE_BUILDER_HPP
#pragma once

#include "scene.hpp"
#include "world_parser.hpp"
#include "xac.hpp"
#include "ipf_manager.hpp"
#include <unordered_map>
#include <algorithm>

namespace loader
{
    class SceneLoader
    {
    public:
        SceneLoader(std::shared_ptr<IPFManager> ipf,
                    std::unordered_map<std::string, std::string> mesh_map)
            : ipf_(std::move(ipf)), mesh_map_(std::move(mesh_map)) {}

        std::vector<scene::Scene> load(const std::string &path,
                                       const std::vector<uint8_t> &data)
        {
            std::string ext = get_ext(path);

            if (ext == "3dworld")
                return load_world(data);

            if (ext == "xac")
                return {load_xac(path, data)};

            throw std::runtime_error("Unsupported format: " + ext);
        }

    private:
        std::shared_ptr<IPFManager> ipf_;
        std::unordered_map<std::string, std::string> mesh_map_;

        // ------------------- WORLD -------------------
        std::vector<scene::Scene> load_world(const std::vector<uint8_t> &data)
        {
            world_parser::World world =
                world_parser::World::from_string(std::string(data.begin(), data.end()));

            std::vector<scene::Scene> scenes;

            for (const auto &model : world.models)
            {
                std::vector<uint8_t> bytes;
                bool found = false;

                std::string file = normalize(model.file);

                for (const auto &dir : world.model_dirs)
                {
                    std::string base = normalize(dir.path);

                    std::vector<std::string> ipfs = {
                        normalize(dir.ipf_name),
                        "bg_hi2",
                        "bg_hi3"};

                    for (const auto &ipf : ipfs)
                    {
                        std::vector<std::string> candidates = {
                            ipf + "/" + base + "/" + file,
                            ipf + "/" + file};

                        for (auto &p : candidates)
                        {
                            std::string full_path = normalize(p);

                            if (ipf_->exists(full_path))
                            {
                                bytes = ipf_->extract(full_path);
                                found = true;
                                break;
                            }
                        }

                        if (found)
                            break;
                    }

                    if (found)
                        break;
                }

                if (!found)
                {
                    throw std::runtime_error("Model not found in any IPF: " + model.file);
                }

                xac::XACRoot xac_root = xac::XACRoot::from_bytes(bytes);

                scene::Scene s = scene::Scene::from_xac_root(xac_root, "");

                apply_transform(model, s);

                scenes.push_back(std::move(s));
            }

            return scenes;
        }

        // ------------------- XAC -------------------
        scene::Scene load_xac(const std::string &full_path,
                              const std::vector<uint8_t> &data)
        {
            xac::XACRoot xac_root = xac::XACRoot::from_bytes(data);

            std::string key = to_lower(full_path);

            std::string texture_path = "";
            if (auto it = mesh_map_.find(key); it != mesh_map_.end())
                texture_path = it->second;

            return scene::Scene::from_xac_root(xac_root, texture_path);
        }

        // ------------------- HELPERS -------------------

        static std::string normalize(std::string s)
        {
            std::replace(s.begin(), s.end(), '\\', '/');

            size_t pos = 0;
            while ((pos = s.find("//", pos)) != std::string::npos)
                s.erase(pos, 1);

            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            return s;
        }

        static std::string get_ext(const std::string &p)
        {
            auto pos = p.find_last_of('.');
            return pos == std::string::npos ? "" : p.substr(pos + 1);
        }

        static std::string to_lower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            return s;
        }

        static void apply_transform(const world_parser::Model &m,
                                    scene::Scene &s)
        {
            if (m.pos)
                s.position = scene::dx_to_gl_position(parse_vec3(*m.pos));
            if (m.rot)
                s.rotation = scene::dx_to_gl_quat(parse_quat(*m.rot));
            if (m.scale)
                s.scale = scene::dx_to_gl_scale(parse_vec3(*m.scale));
        }

        static scene::Vector3 parse_vec3(const std::string &str)
        {
            scene::Vector3 v{};
            std::sscanf(str.c_str(), "%f,%f,%f", &v.x, &v.y, &v.z);
            return v;
        }

        static scene::Vector4 parse_quat(const std::string &str)
        {
            scene::Vector4 q{};
            std::sscanf(str.c_str(), "%f,%f,%f,%f", &q.x, &q.y, &q.z, &q.w);
            return q;
        }
    };
}

#endif