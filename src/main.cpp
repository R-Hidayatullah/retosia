#include <iostream>
#include <chrono>
#include "ipf.hpp" // Your full IPF parser

int main()
{
    using clock = std::chrono::high_resolution_clock;

    std::filesystem::path game_root =
        R"(C:\Program Files (x86)\Steam\steamapps\common\TreeOfSavior)";

    auto start_total = clock::now();

    // 1️⃣ Parse IPFs in data/
    auto start_data_parse = clock::now();
    auto data_ipfs = ipf::parse_all_ipf_files_limited_threads(game_root / "data", 8);
    auto end_data_parse = clock::now();
    std::cout << "[Step 1] Parsing data/ IPFs: "
              << std::chrono::duration<double>(end_data_parse - start_data_parse).count()
              << " seconds\n";

    // 2️⃣ Parse IPFs in patch/
    auto start_patch_parse = clock::now();
    auto patch_ipfs = ipf::parse_all_ipf_files_limited_threads(game_root / "patch", 8);
    auto end_patch_parse = clock::now();
    std::cout << "[Step 2] Parsing patch/ IPFs: "
              << std::chrono::duration<double>(end_patch_parse - start_patch_parse).count()
              << " seconds\n";

    // Merge all IPFs
    auto start_merge = clock::now();
    data_ipfs.insert(data_ipfs.end(),
                     std::make_move_iterator(patch_ipfs.begin()),
                     std::make_move_iterator(patch_ipfs.end()));
    auto end_merge = clock::now();
    std::cout << "[Step 3] Merging IPFs: "
              << std::chrono::duration<double>(end_merge - start_merge).count()
              << " seconds\n";

    // 4️⃣ Collect file tables
    auto start_collect = clock::now();
    auto all_files = ipf::collect_file_tables(data_ipfs);
    auto end_collect = clock::now();
    std::cout << "[Step 4] Collecting file tables: "
              << std::chrono::duration<double>(end_collect - start_collect).count()
              << " seconds\n";

    // 5️⃣ Sort file tables
    auto start_sort = clock::now();
    ipf::sort_file_tables_by_folder_then_name(all_files);
    auto end_sort = clock::now();
    std::cout << "[Step 5] Sorting file tables: "
              << std::chrono::duration<double>(end_sort - start_sort).count()
              << " seconds\n";

    // 6️⃣ Locate target file
    const std::string target = "char_hi/misc/box.xac";
    auto start_find = clock::now();
    auto it = std::find_if(all_files.begin(), all_files.end(),
                           [&target](const ipf::IPFFileTable &entry)
                           {
                               return entry.directory_name == target;
                           });
    auto end_find = clock::now();
    std::cout << "[Step 6] Searching for file: "
              << std::chrono::duration<double>(end_find - start_find).count()
              << " seconds\n";

    if (it != all_files.end())
    {
        // 7️⃣ Extract file
        auto start_extract = clock::now();
        try
        {
            auto data = it->extract_data();
            auto end_extract = clock::now();

            std::cout << "[Step 7] Extracted '" << target
                      << "' size = " << data.size() << " bytes\n";
            std::cout << "[Step 7] Extraction time: "
                      << std::chrono::duration<double>(end_extract - start_extract).count()
                      << " seconds\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to extract: " << e.what() << "\n";
        }
    }
    else
    {
        std::cout << "File not found: " << target << "\n";
    }

    auto end_total = clock::now();
    std::cout << "[Total] Program runtime: "
              << std::chrono::duration<double>(end_total - start_total).count()
              << " seconds\n";

    return 0;
}