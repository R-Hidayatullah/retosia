#include "xac.hpp"
#include "scene.hpp"
#include <iostream>
#include <chrono>

int main()
{
    try
    {
        const std::string path = "tests/npc_lecifer_set.xac"; // <-- change to your file
        const std::string texture_path = "";

        auto start = std::chrono::high_resolution_clock::now();

        // 1. Load XAC
        xac::XACRoot xac_root = xac::XACRoot::from_file(path);

        auto end_load = std::chrono::high_resolution_clock::now();
        std::cout << "Loaded XAC in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end_load - start).count()
                  << " ms\n";

        // ------------------ Print XAC info ------------------
        std::cout << "\nXAC Chunks:\n";
        for (size_t i = 0; i < xac_root.chunks.size(); ++i)
        {
            const auto &chunk = xac_root.chunks[i];
            std::cout << "Chunk #" << i
                      << " ID=" << chunk.chunk.chunk_id
                      << " Version=" << chunk.chunk.version
                      << "\n";

            if (chunk.chunk.chunk_id == static_cast<uint32_t>(xac::XACChunkId::Mesh))
            {
                if (chunk.chunk.version == 1)
                {
                    if (auto *mesh = std::get_if<xac::XACMesh>(&chunk.chunk_data))
                    {
                        std::cout << "  Submeshes: " << mesh->sub_meshes.size() << "\n";
                        for (size_t s = 0; s < mesh->sub_meshes.size(); ++s)
                        {
                            const auto &sm = mesh->sub_meshes[s];
                            std::cout << "    Submesh #" << s
                                      << " verts=" << sm.num_verts
                                      << " indices=" << sm.num_indices
                                      << " material_index=" << sm.material_index
                                      << "\n";
                        }
                    }
                }
                else if (chunk.chunk.version == 2)
                {
                    if (auto *mesh = std::get_if<xac::XACMesh2>(&chunk.chunk_data))
                    {
                        std::cout << "  Submeshes: " << mesh->sub_meshes.size() << "\n";
                        for (size_t s = 0; s < mesh->sub_meshes.size(); ++s)
                        {
                            const auto &sm = mesh->sub_meshes[s];
                            std::cout << "    Submesh #" << s
                                      << " verts=" << sm.num_verts
                                      << " indices=" << sm.num_indices
                                      << " material_index=" << sm.material_index
                                      << "\n";
                        }
                    }
                }
            }
        }

        // 2. Convert to Scene
        scene::Scene scene = scene::Scene::from_xac_root(xac_root, texture_path);

        auto end_scene = std::chrono::high_resolution_clock::now();
        std::cout << "\nConverted to Scene in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end_scene - end_load).count()
                  << " ms\n";

        // 3. Debug print Scene
        std::cout << "\nScene Root Nodes: " << scene.root_nodes.size() << "\n";

        for (const auto &node : scene.root_nodes)
        {
            std::cout << "Node: " << node.name << "\n";

            if (node.position)
                std::cout << "  Position: (" << node.position->x << ", " << node.position->y << ", " << node.position->z << ")\n";
            if (node.rotation)
                std::cout << "  Rotation: (" << node.rotation->x << ", " << node.rotation->y << ", " << node.rotation->z << ", " << node.rotation->w << ")\n";
            if (node.scale)
                std::cout << "  Scale   : (" << node.scale->x << ", " << node.scale->y << ", " << node.scale->z << ")\n";

            if (node.model)
            {
                std::cout << "  Model: " << node.model->name << "\n";
                std::cout << "    Submeshes: " << node.model->submeshes.size() << "\n";

                for (const auto &sm : node.model->submeshes)
                {
                    std::cout << "      Submesh vertices: " << sm.positions.size()
                              << " indices: " << sm.indices.size()
                              << " texture: " << sm.textures << "\n";

                    if (!sm.positions.empty())
                        std::cout << "        First vertex: (" << sm.positions[0].x << ", " << sm.positions[0].y << ", " << sm.positions[0].z << ")\n";
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}