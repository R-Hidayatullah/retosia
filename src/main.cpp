#include <iostream>
#include "ipf.hpp"

int main()
{
    try
    {
        // Parse a single .ipf file
        ipf::IPFRoot root = ipf::IPFRoot::from_file("tests/xml_client.ipf");

        // Iterate over all files
        for (auto &entry : root.file_table)
        {
            std::cout << entry.directory_name
                      << " (compressed: " << entry.file_size_compressed
                      << ", uncompressed: " << entry.file_size_uncompressed << ")\n";

            // Extract file bytes
            std::vector<uint8_t> data = entry.extract_data();

            // Optionally: print a hex preview
            ipf::print_hex_viewer(data, std::cout);
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}