#pragma once
#ifndef TSV_HPP
#define TSV_HPP

// ============================================================
//  tsv.hpp  –  TSV file parser (C++20 port of tsv.rs)
// ============================================================
//  Dependencies: standard library only
//  Requires:     C++20
// ============================================================

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <future>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace tsv
{

    // -------------------------------------------------------
    //  TSVParser
    //  Stateless helper class for parsing tab-separated files.
    // -------------------------------------------------------
    class TSVParser
    {
    public:
        // Parse a single TSV file into a 2-D grid of strings.
        static std::vector<std::vector<std::string>>
        parse_file(const std::filesystem::path &path)
        {
            std::ifstream file(path);
            if (!file)
                throw std::runtime_error("Cannot open TSV file: " + path.string());

            std::vector<std::vector<std::string>> rows;
            std::string line;
            while (std::getline(file, line))
            {
                std::vector<std::string> cols;
                std::istringstream ss(line);
                std::string col;
                while (std::getline(ss, col, '\t'))
                    cols.push_back(col);
                rows.push_back(std::move(cols));
            }
            return rows;
        }

        // Parse ETC.tsv and ITEM.tsv from a language folder in parallel.
        // Returns { etc_data, item_data }.
        static std::pair<std::vector<std::vector<std::string>>,
                         std::vector<std::vector<std::string>>>
        parse_language_data(const std::filesystem::path &lang_folder)
        {
            auto etc_path = lang_folder / "ETC.tsv";
            auto item_path = lang_folder / "ITEM.tsv";

            auto fut_etc = std::async(std::launch::async,
                                      [&]
                                      { return parse_file(etc_path); });
            auto fut_item = std::async(std::launch::async,
                                       [&]
                                       { return parse_file(item_path); });

            return {fut_etc.get(), fut_item.get()};
        }
    };

} // namespace tsv

#endif // TSV_HPP