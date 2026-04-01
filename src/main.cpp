#include <iostream>
#include <chrono>
#include <unordered_map>
#include <thread> // for hardware_concurrency
#include "ipf.hpp"

int main()
{
    std::filesystem::path game_root =
        R"(C:\Program Files (x86)\Steam\steamapps\common\TreeOfSavior)";

    auto t0 = std::chrono::high_resolution_clock::now();

    // Detect max threads from CPU
    unsigned max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0) // fallback if detection fails
        max_threads = 4;

    std::cout << "[Info] Using " << max_threads << " threads for IPF parsing\n";

    // 1️⃣ Parse all IPFs, keep latest version per file
    auto latest_files = ipf::parse_game_ipfs_latest_streamed(game_root, max_threads);
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "[Step 1] Parsed latest IPFs: " << latest_files.size()
              << " files in " << std::chrono::duration<double>(t1 - t0).count() << "s\n";

    // 2️⃣ Search for a specific file
    const std::string target = "ies_client/xac.ies";
    auto it = latest_files.find(target);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "[Step 2] Search time: " << std::chrono::duration<double>(t2 - t1).count() << "s\n";

    if (it != latest_files.end())
    {
        // ✅ Print which IPF file this entry came from
        std::cout << "[Step 2] Found in archive: "
                  << it->second.container_name << " ("
                  << it->second.file_path->string() << ")\n";

        // 3️⃣ Extract
        auto t3_start = std::chrono::high_resolution_clock::now();
        auto data = it->second.extract_data();
        auto t3_end = std::chrono::high_resolution_clock::now();

        std::cout << "[Step 3] Extracted '" << target << "' size = "
                  << data.size() << " bytes\n";
        std::cout << "[Step 3] Extraction time: "
                  << std::chrono::duration<double>(t3_end - t3_start).count() << "s\n";
    }
    else
    {
        std::cout << "File not found: " << target << "\n";
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    std::cout << "[Total] Program runtime: "
              << std::chrono::duration<double>(t_end - t0).count() << "s\n";

    std::cout << "Press Enter to exit...";
    std::cin.get();
}