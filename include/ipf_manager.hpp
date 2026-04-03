#ifndef IPF_MANAGER_HPP
#define IPF_MANAGER_HPP

#pragma once
#include "ipf.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <algorithm>
#include <filesystem>

namespace loader
{
    class IPFManager
    {
    public:
        explicit IPFManager(const std::filesystem::path &root, tp::ThreadPool &pool)
        {
            auto raw_files = ipf::parse_game_ipfs_latest_streamed(root, pool);

            for (auto &kv : raw_files)
            {
                files_.emplace(normalize_path(kv.first), std::move(kv.second));
            }
        }

        // ---------- Fast lookup ----------
        const ipf::IPFFileTable *find_latest(const std::string &path) const
        {
            auto it = files_.find(normalize_path(path));
            if (it == files_.end())
                return nullptr;
            return &it->second;
        }

        bool exists(const std::string &path) const
        {
            return find_latest(path) != nullptr;
        }

        std::vector<uint8_t> extract(const std::string &path) const
        {
            auto file = find_latest(path);
            if (!file)
                throw std::runtime_error("File not found: " + path);

            return file->extract_data();
        }

    private:
        std::unordered_map<std::string, ipf::IPFFileTable> files_;

        static std::string normalize_path(std::string path)
        {
            // slash normalize
            std::replace(path.begin(), path.end(), '\\', '/');

            // remove duplicate slashes
            size_t pos = 0;
            while ((pos = path.find("//", pos)) != std::string::npos)
                path.erase(pos, 1);

            // lowercase
            std::transform(path.begin(), path.end(), path.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            return path;
        }
    };
}

#endif