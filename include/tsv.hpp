#pragma once
#ifndef TSV_HPP
#define TSV_HPP

#include "thread_pool.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <future>
#include <utility>

namespace tsv
{

    class TSVParser
    {
    public:
        // Parse a single TSV file into a 2-D grid of strings
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

        // Parse ETC.tsv and ITEM.tsv in parallel using a ThreadPool
        static std::pair<std::vector<std::vector<std::string>>,
                         std::vector<std::vector<std::string>>>
        parse_language_data(const std::filesystem::path &lang_folder)
        {
            auto etc_path = lang_folder / "ETC.tsv";
            auto item_path = lang_folder / "ITEM.tsv";

            tp::ThreadPool pool(2); // 2 threads are enough for ETC + ITEM

            auto fut_etc = pool.submit(parse_file, etc_path);
            auto fut_item = pool.submit(parse_file, item_path);

            pool.wait_all(); // optional, ensures tasks finish before continuing

            return {fut_etc.get(), fut_item.get()};
        }
    };

} // namespace tsv

#endif // TSV_HPP