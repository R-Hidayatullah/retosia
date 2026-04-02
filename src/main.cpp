#include "world_parser.hpp" // your single-header file
#include <iostream>

int main()
{
    try
    {
        // Load World from file
        auto world = world_parser::World::from_file("tests/barrack.3dworld");

        // Print Model Directories
        std::cout << "Model Directories:\n";
        for (const auto &md : world.model_dirs)
        {
            std::cout << "  IpfName: " << md.ipf_name << ", Path: " << md.path << "\n";
        }

        // Print Texture Directories
        std::cout << "\nTexture Directories:\n";
        for (const auto &td : world.tex_dirs)
        {
            std::cout << "  IpfName: " << td.ipf_name << ", Path: " << td.path << "\n";
        }

        // Print Models
        std::cout << "\nModels:\n";
        for (const auto &m : world.models)
        {
            std::cout << "  File: " << m.file
                      << ", Model: " << m.model;
            if (m.pos)
                std::cout << ", Pos: " << *m.pos;
            if (m.rot)
                std::cout << ", Rot: " << *m.rot;
            if (m.scale)
                std::cout << ", Scale: " << *m.scale;
            std::cout << "\n";
        }

        // Print StandOnPos if present
        if (world.stand_on_pos)
        {
            std::cout << "\nStandOnPos: " << world.stand_on_pos->pos << "\n";
        }

        std::cout << "\nLightMaps:\n";
        for (const auto &lm : world.light_maps)
        {
            std::cout << "  File: " << lm.file;
            if (lm.length)
                std::cout << ", Length: " << *lm.length;
            if (lm.offset)
                std::cout << ", Offset: " << *lm.offset;
            if (lm.size)
                std::cout << ", Size: " << *lm.size;
            std::cout << "\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading .3dworld file: " << e.what() << "\n";
        return 1;
    }

    return 0;
}