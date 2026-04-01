#include <iostream>
#include "xac.hpp"

int main()
{
    try
    {
        // Parse an XAC file from disk
        xac::XACRoot actor = xac::XACRoot::from_file("tests/boss_wastrel_set.xac");

        // Print some info
        std::cout << "XAC file parsed successfully.\n";
        std::cout << "Number of chunks: " << actor.chunks.size() << "\n";

        // Collect all texture names referenced in this actor
        auto textures = actor.get_texture_names();
        std::cout << "Textures used:\n";
        for (const auto &t : textures)
            std::cout << "  " << t << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}