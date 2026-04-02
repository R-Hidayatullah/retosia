#if !defined(IPF_MANAGER_HPP)
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
            // Parse IPFs and normalize paths for consistent lookup
            auto raw_files = ipf::parse_game_ipfs_latest_streamed(root, pool);
            for (auto &kv : raw_files)
            {
                files_.emplace(normalize_path(kv.first), std::move(kv.second));
            }
        }

        // Find latest file (normalized path)
        const ipf::IPFFileTable *find_latest(const std::string &path) const
        {
            auto it = files_.find(normalize_path(path));
            if (it == files_.end())
                return nullptr;
            return &it->second;
        }

        // Extract file data by normalized path
        std::vector<uint8_t> extract(const std::string &path) const
        {
            auto file = find_latest(path);
            if (!file)
                throw std::runtime_error("File not found: " + path);

            return file->extract_data();
        }

    private:
        std::unordered_map<std::string, ipf::IPFFileTable> files_;

        // Normalize path: lowercase, forward slashes, remove duplicate slashes
        static std::string normalize_path(const std::string &path)
        {
            std::string result = path;

            // Replace backslashes with forward slashes
            std::replace(result.begin(), result.end(), '\\', '/');

            // Remove duplicate slashes
            size_t pos = 0;
            while ((pos = result.find("//", pos)) != std::string::npos)
                result.erase(pos, 1);

            // Convert to lowercase for case-insensitive lookup
            std::transform(result.begin(), result.end(), result.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            return result;
        }
    };
}

#endif // IPF_MANAGER_HPP