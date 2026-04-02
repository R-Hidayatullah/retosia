#include "scene_builder.hpp" // WorldSceneBuilder
#include "scene.hpp"         // scene::Scene
#include <iostream>
#include <vector>
#include "ies.hpp"

void print_scene_node(const scene::SceneNode &node, int depth = 0)
{
    std::string indent(depth * 2, ' ');

    std::cout << indent << "Node: " << node.name;
    if (node.model)
        std::cout << " (Model: " << node.model->submeshes.size() << " submeshes, Name: "
                  << node.model->name << ")";
    std::cout << "\n";

    if (node.model)
    {
        int sm_idx = 0;
        for (const auto &sub : node.model->submeshes)
        {
            std::cout << indent << "  SubMesh #" << sm_idx++
                      << " | Vertices: " << sub.positions.size()
                      << " | Indices: " << sub.indices.size()
                      << " | Texture: " << sub.textures << "\n";
        }
    }

    for (const auto &child : node.children)
        print_scene_node(child, depth + 1);
}

void print_scene_summary(const scene::Scene &scene)
{
    std::cout << "Scene summary:\n";
    std::cout << "Root nodes: " << scene.root_nodes.size() << "\n";

    int total_submeshes = 0;
    int total_vertices = 0;
    int total_indices = 0;
    std::cout << "\n\n";

    for (const auto &node : scene.root_nodes)
    {
        std::cout << "\n\n";
        if (node.model)
        {
            total_submeshes += node.model->submeshes.size();
            for (const auto &sub : node.model->submeshes)
            {
                total_vertices += sub.positions.size();
                total_indices += sub.indices.size();
            }
        }
        // Also print node tree
        print_scene_node(node);
    }

    std::cout << "Total submeshes: " << total_submeshes << "\n";
    std::cout << "Total vertices: " << total_vertices << "\n";
    std::cout << "Total indices: " << total_indices << "\n";
}

int main()
{
    try
    {
        std::filesystem::path game_root = R"(C:\Program Files (x86)\Steam\steamapps\common\TreeOfSavior)";
        tp::ThreadPool pool;

        // 1. Load IPFs
        auto ipf_manager = std::make_shared<loader::IPFManager>(game_root, pool);

        // 2. Build mesh_map (from IES)
        std::unordered_map<std::string, std::string> mesh_map;
        {
            auto data = ipf_manager->extract("ies_client/xac.ies");
            auto ies = ies::IESRoot::from_bytes(data);
            mesh_map = ies.extract_mesh_path_map();
        }

        // 3. Create loader
        loader::SceneLoader loader(ipf_manager, mesh_map);

        // 4. Load file (like API preview)
        std::string path = "bg/hi_entity/barrack.3dworld";
        auto data = ipf_manager->extract(path);

        auto scenes = loader.load(path, data);

        std::cout << "Loaded scenes: " << scenes.size() << "\n";
        for (size_t i = 0; i < scenes.size(); ++i)
        {
            std::cout << "Scene #" << i << ":\n";
            print_scene_summary(scenes[i]);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
}