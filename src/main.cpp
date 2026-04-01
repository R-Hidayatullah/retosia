#include <iostream>
#include <filesystem>
#include <chrono>
#include "ipf.hpp"

int main()
{
    std::filesystem::path game_root =
        R"(C:\Program Files (x86)\Steam\steamapps\common\TreeOfSavior)";

    auto total_start = std::chrono::high_resolution_clock::now();

    // ---------------- Step 1: Parse latest IPFs ----------------
    auto t1_start = std::chrono::high_resolution_clock::now();
    auto latest_files = ipf::parse_latest_ipfs(game_root, 8); // 8 threads
    auto t1_end = std::chrono::high_resolution_clock::now();

    std::cout << "[Step 1] Parsing latest IPFs: "
              << std::chrono::duration<double>(t1_end - t1_start).count()
              << " seconds, total files = " << latest_files.size() << "\n";

    // ---------------- Step 2: Search for a target file ----------------
    const std::string target = "char_hi/misc/box.xac";
    auto t2_start = std::chrono::high_resolution_clock::now();
    auto it = latest_files.find(target);
    auto t2_end = std::chrono::high_resolution_clock::now();

    std::cout << "[Step 2] Searching for file: "
              << std::chrono::duration<double>(t2_end - t2_start).count()
              << " seconds\n";

    // ---------------- Step 3: Extract file (if found) ----------------
    if (it != latest_files.end())
    {
        auto t3_start = std::chrono::high_resolution_clock::now();
        auto data = it->second.extract_data();
        auto t3_end = std::chrono::high_resolution_clock::now();

        std::cout << "[Step 3] Extracted '" << target
                  << "' size = " << data.size() << " bytes\n";
        std::cout << "[Step 3] Extraction time: "
                  << std::chrono::duration<double>(t3_end - t3_start).count()
                  << " seconds\n";

        // Optional: hex viewer
    }
    else
    {
        std::cout << "File not found: " << target << "\n";
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    std::cout << "[Total] Program runtime: "
              << std::chrono::duration<double>(total_end - total_start).count()
              << " seconds\n";

    return 0;
}