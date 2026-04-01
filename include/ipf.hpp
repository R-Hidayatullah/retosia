#pragma once
#ifndef IPF_HPP
#define IPF_HPP

// ============================================================
//  ipf.hpp  –  IPF archive parser (C++20 port of ipf.rs)
// ============================================================
//  IPF is the archive format used by Tree of Savior.
//  Files are stored with a PKzip-like footer, encrypted with a
//  custom key-derivation scheme and compressed with raw DEFLATE.
//
//  Layout:
//    [file data blobs]
//    [file table: IPFFileTable entries]
//    [footer: IPFHeader, located at End-24]
//
//  Dependencies:
//    - zlib  (link with -lz)   for raw DEFLATE decompression
//    - C++ standard library
//  Requires: C++20
// ============================================================

#include "binary_reader.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <iostream>
#include <map>

// zlib for raw DEFLATE
#include <zlib.h>

namespace ipf
{

    // -------------------------------------------------------
    //  Constants
    // -------------------------------------------------------
    inline constexpr int64_t HEADER_LOCATION = -24;
    inline constexpr uint32_t MAGIC_NUMBER = 0x06054B50;

    inline constexpr uint32_t CRC32_TABLE[256] = {
        0x00000000,
        0x77073096,
        0xee0e612c,
        0x990951ba,
        0x076dc419,
        0x706af48f,
        0xe963a535,
        0x9e6495a3,
        0x0edb8832,
        0x79dcb8a4,
        0xe0d5e91e,
        0x97d2d988,
        0x09b64c2b,
        0x7eb17cbd,
        0xe7b82d07,
        0x90bf1d91,
        0x1db71064,
        0x6ab020f2,
        0xf3b97148,
        0x84be41de,
        0x1adad47d,
        0x6ddde4eb,
        0xf4d4b551,
        0x83d385c7,
        0x136c9856,
        0x646ba8c0,
        0xfd62f97a,
        0x8a65c9ec,
        0x14015c4f,
        0x63066cd9,
        0xfa0f3d63,
        0x8d080df5,
        0x3b6e20c8,
        0x4c69105e,
        0xd56041e4,
        0xa2677172,
        0x3c03e4d1,
        0x4b04d447,
        0xd20d85fd,
        0xa50ab56b,
        0x35b5a8fa,
        0x42b2986c,
        0xdbbbc9d6,
        0xacbcf940,
        0x32d86ce3,
        0x45df5c75,
        0xdcd60dcf,
        0xabd13d59,
        0x26d930ac,
        0x51de003a,
        0xc8d75180,
        0xbfd06116,
        0x21b4f4b5,
        0x56b3c423,
        0xcfba9599,
        0xb8bda50f,
        0x2802b89e,
        0x5f058808,
        0xc60cd9b2,
        0xb10be924,
        0x2f6f7c87,
        0x58684c11,
        0xc1611dab,
        0xb6662d3d,
        0x76dc4190,
        0x01db7106,
        0x98d220bc,
        0xefd5102a,
        0x71b18589,
        0x06b6b51f,
        0x9fbfe4a5,
        0xe8b8d433,
        0x7807c9a2,
        0x0f00f934,
        0x9609a88e,
        0xe10e9818,
        0x7f6a0dbb,
        0x086d3d2d,
        0x91646c97,
        0xe6635c01,
        0x6b6b51f4,
        0x1c6c6162,
        0x856530d8,
        0xf262004e,
        0x6c0695ed,
        0x1b01a57b,
        0x8208f4c1,
        0xf50fc457,
        0x65b0d9c6,
        0x12b7e950,
        0x8bbeb8ea,
        0xfcb9887c,
        0x62dd1ddf,
        0x15da2d49,
        0x8cd37cf3,
        0xfbd44c65,
        0x4db26158,
        0x3ab551ce,
        0xa3bc0074,
        0xd4bb30e2,
        0x4adfa541,
        0x3dd895d7,
        0xa4d1c46d,
        0xd3d6f4fb,
        0x4369e96a,
        0x346ed9fc,
        0xad678846,
        0xda60b8d0,
        0x44042d73,
        0x33031de5,
        0xaa0a4c5f,
        0xdd0d7cc9,
        0x5005713c,
        0x270241aa,
        0xbe0b1010,
        0xc90c2086,
        0x5768b525,
        0x206f85b3,
        0xb966d409,
        0xce61e49f,
        0x5edef90e,
        0x29d9c998,
        0xb0d09822,
        0xc7d7a8b4,
        0x59b33d17,
        0x2eb40d81,
        0xb7bd5c3b,
        0xc0ba6cad,
        0xedb88320,
        0x9abfb3b6,
        0x03b6e20c,
        0x74b1d29a,
        0xead54739,
        0x9dd277af,
        0x04db2615,
        0x73dc1683,
        0xe3630b12,
        0x94643b84,
        0x0d6d6a3e,
        0x7a6a5aa8,
        0xe40ecf0b,
        0x9309ff9d,
        0x0a00ae27,
        0x7d079eb1,
        0xf00f9344,
        0x8708a3d2,
        0x1e01f268,
        0x6906c2fe,
        0xf762575d,
        0x806567cb,
        0x196c3671,
        0x6e6b06e7,
        0xfed41b76,
        0x89d32be0,
        0x10da7a5a,
        0x67dd4acc,
        0xf9b9df6f,
        0x8ebeeff9,
        0x17b7be43,
        0x60b08ed5,
        0xd6d6a3e8,
        0xa1d1937e,
        0x38d8c2c4,
        0x4fdff252,
        0xd1bb67f1,
        0xa6bc5767,
        0x3fb506dd,
        0x48b2364b,
        0xd80d2bda,
        0xaf0a1b4c,
        0x36034af6,
        0x41047a60,
        0xdf60efc3,
        0xa867df55,
        0x316e8eef,
        0x4669be79,
        0xcb61b38c,
        0xbc66831a,
        0x256fd2a0,
        0x5268e236,
        0xcc0c7795,
        0xbb0b4703,
        0x220216b9,
        0x5505262f,
        0xc5ba3bbe,
        0xb2bd0b28,
        0x2bb45a92,
        0x5cb36a04,
        0xc2d7ffa7,
        0xb5d0cf31,
        0x2cd99e8b,
        0x5bdeae1d,
        0x9b64c2b0,
        0xec63f226,
        0x756aa39c,
        0x026d930a,
        0x9c0906a9,
        0xeb0e363f,
        0x72076785,
        0x05005713,
        0x95bf4a82,
        0xe2b87a14,
        0x7bb12bae,
        0x0cb61b38,
        0x92d28e9b,
        0xe5d5be0d,
        0x7cdcefb7,
        0x0bdbdf21,
        0x86d3d2d4,
        0xf1d4e242,
        0x68ddb3f8,
        0x1fda836e,
        0x81be16cd,
        0xf6b9265b,
        0x6fb077e1,
        0x18b74777,
        0x88085ae6,
        0xff0f6a70,
        0x66063bca,
        0x11010b5c,
        0x8f659eff,
        0xf862ae69,
        0x616bffd3,
        0x166ccf45,
        0xa00ae278,
        0xd70dd2ee,
        0x4e048354,
        0x3903b3c2,
        0xa7672661,
        0xd06016f7,
        0x4969474d,
        0x3e6e77db,
        0xaed16a4a,
        0xd9d65adc,
        0x40df0b66,
        0x37d83bf0,
        0xa9bcae53,
        0xdebb9ec5,
        0x47b2cf7f,
        0x30b5ffe9,
        0xbdbdf21c,
        0xcabac28a,
        0x53b39330,
        0x24b4a3a6,
        0xbad03605,
        0xcdd70693,
        0x54de5729,
        0x23d967bf,
        0xb3667a2e,
        0xc4614ab8,
        0x5d681b02,
        0x2a6f2b94,
        0xb40bbe37,
        0xc30c8ea1,
        0x5a05df1b,
        0x2d02ef8d,
    };

    inline constexpr uint8_t PASSWORD[20] = {
        0x6F,
        0x66,
        0x4F,
        0x31,
        0x61,
        0x30,
        0x75,
        0x65,
        0x58,
        0x41,
        0x3F,
        0x20,
        0x5B,
        0xFF,
        0x73,
        0x20,
        0x68,
        0x20,
        0x25,
        0x3F,
    };

    // -------------------------------------------------------
    //  IPF footer / header  (located at file_end - 24)
    // -------------------------------------------------------
    struct IPFHeader
    {
        uint16_t file_count{0};
        uint32_t file_table_pointer{0};
        uint16_t padding{0};
        uint32_t header_pointer{0};
        uint32_t magic{0}; // must equal MAGIC_NUMBER
        uint32_t version_to_patch{0};
        uint32_t new_version{0};

        void read(binreader::BinaryReader &r)
        {
            file_count = r.read<uint16_t>();
            file_table_pointer = r.read<uint32_t>();
            padding = r.read<uint16_t>();
            header_pointer = r.read<uint32_t>();
            magic = r.read<uint32_t>();
            version_to_patch = r.read<uint32_t>();
            new_version = r.read<uint32_t>();
            if (magic != MAGIC_NUMBER)
                throw std::runtime_error("IPF: magic mismatch – file may be corrupted");
        }
    };

    // -------------------------------------------------------
    //  Decryption helpers (static, no state)
    // -------------------------------------------------------
    namespace detail
    {
        inline uint32_t crc32_byte(uint32_t crc, uint8_t b)
        {
            return CRC32_TABLE[((crc ^ static_cast<uint32_t>(b)) & 0xFF)] ^ (crc >> 8);
        }

        inline uint8_t extract_byte(uint32_t value, int byte_index)
        {
            return static_cast<uint8_t>(value >> (byte_index * 8));
        }

        inline void update_keys(uint32_t (&keys)[3], uint8_t byte)
        {
            keys[0] = crc32_byte(keys[0], byte);
            keys[1] = 0x8088405u * (static_cast<uint32_t>(static_cast<uint8_t>(keys[0])) + keys[1]) + 1;
            keys[2] = crc32_byte(keys[2], extract_byte(keys[1], 3));
        }

        inline void generate_keys(uint32_t (&keys)[3])
        {
            keys[0] = 0x12345678;
            keys[1] = 0x23456789;
            keys[2] = 0x34567890;
            for (uint8_t b : PASSWORD)
                update_keys(keys, b);
        }

        inline void decrypt_in_place(std::vector<uint8_t> &buf)
        {
            if (buf.empty())
                return;
            uint32_t keys[3];
            generate_keys(keys);
            std::size_t steps = (buf.size() - 1) / 2 + 1;
            for (std::size_t i = 0; i < steps; ++i)
            {
                uint32_t v = (keys[2] & 0xFFFD) | 2;
                std::size_t idx = i * 2;
                if (idx < buf.size())
                {
                    buf[idx] ^= static_cast<uint8_t>((v * (v ^ 1u)) >> 8);
                    update_keys(keys, buf[idx]);
                }
            }
        }

        inline std::vector<uint8_t> decompress_deflate(const std::vector<uint8_t> &src,
                                                       std::size_t uncompressed_size)
        {
            std::vector<uint8_t> dst(uncompressed_size);
            z_stream zs{};
            zs.next_in = const_cast<Bytef *>(src.data());
            zs.avail_in = static_cast<uInt>(src.size());
            zs.next_out = dst.data();
            zs.avail_out = static_cast<uInt>(dst.size());

            // -15 = raw DEFLATE (no zlib header)
            if (inflateInit2(&zs, -15) != Z_OK)
                throw std::runtime_error("IPF: zlib inflateInit2 failed");

            int ret = inflate(&zs, Z_FINISH);
            inflateEnd(&zs);

            if (ret != Z_STREAM_END)
                throw std::runtime_error("IPF: zlib inflate failed (corrupted data?)");

            dst.resize(zs.total_out);
            return dst;
        }

        inline bool should_skip_decompression(const std::string &dir_name)
        {
            static constexpr const char *SKIP[] = {".fsb", ".jpg", ".mp3"};
            auto pos = dir_name.rfind('.');
            if (pos == std::string::npos)
                return false;
            std::string ext = dir_name.substr(pos);
            for (auto &c : ext)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            for (auto *s : SKIP)
                if (ext == s)
                    return true;
            return false;
        }
    } // namespace detail

    // -------------------------------------------------------
    //  IPFFileTable  –  one entry in the archive's file table
    // -------------------------------------------------------
    class IPFFileTable
    {
    public:
        uint16_t directory_name_length{0};
        uint32_t crc32{0};
        uint32_t file_size_compressed{0};
        uint32_t file_size_uncompressed{0};
        uint32_t file_pointer{0};
        uint16_t container_name_length{0};
        std::string container_name;
        std::string directory_name;
        std::optional<std::filesystem::path> file_path;

        void read(binreader::BinaryReader &r)
        {
            directory_name_length = r.read<uint16_t>();
            crc32 = r.read<uint32_t>();
            file_size_compressed = r.read<uint32_t>();
            file_size_uncompressed = r.read<uint32_t>();
            file_pointer = r.read<uint32_t>();
            container_name_length = r.read<uint16_t>();

            std::vector<uint8_t> cbuf(container_name_length);
            r.read_bytes(cbuf.data(), container_name_length);
            container_name = std::string(cbuf.begin(), cbuf.end());

            std::vector<uint8_t> dbuf(directory_name_length);
            r.read_bytes(dbuf.data(), directory_name_length);
            directory_name = std::string(dbuf.begin(), dbuf.end());
        }

        // Extract (decrypt + decompress) and return raw bytes.
        // Requires file_path to have been set after construction.
        std::vector<uint8_t> extract_data() const
        {
            if (!file_path)
                throw std::runtime_error("IPF: file_path not set for entry: " + directory_name);

            std::ifstream f(*file_path, std::ios::binary);
            if (!f)
                throw std::runtime_error("IPF: cannot open archive: " + file_path->string());

            f.seekg(file_pointer, std::ios::beg);
            std::vector<uint8_t> buf(file_size_compressed);
            f.read(reinterpret_cast<char *>(buf.data()), file_size_compressed);

            if (!detail::should_skip_decompression(directory_name))
            {
                detail::decrypt_in_place(buf);
                buf = detail::decompress_deflate(buf, file_size_uncompressed);
            }

            return buf;
        }
    };

    // -------------------------------------------------------
    //  IPFRoot  –  the parsed archive
    // -------------------------------------------------------
    class IPFRoot
    {
    public:
        IPFHeader header;
        std::vector<IPFFileTable> file_table;

        // Read the archive from a file path.
        static IPFRoot from_file(const std::filesystem::path &path)
        {
            // Open file to determine size
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f)
                throw std::runtime_error("IPF: cannot open: " + path.string());
            std::streampos file_size = f.tellg();

            // Read the footer header (last 24 bytes)
            f.seekg(static_cast<std::streamoff>(file_size) + HEADER_LOCATION, std::ios::beg);
            std::vector<uint8_t> header_buf(24);
            f.read(reinterpret_cast<char *>(header_buf.data()), 24);
            binreader::BinaryReader hr(std::move(header_buf));

            IPFRoot root;
            root.header.read(hr);

            // Read file table
            f.seekg(root.header.file_table_pointer, std::ios::beg);
            // Read enough bytes for the file table entries (read all remaining)
            std::vector<uint8_t> table_buf(
                static_cast<std::size_t>(file_size) - root.header.file_table_pointer);
            f.read(reinterpret_cast<char *>(table_buf.data()), static_cast<std::streamsize>(table_buf.size()));
            binreader::BinaryReader tr(std::move(table_buf));

            root.file_table.resize(root.header.file_count);
            for (auto &entry : root.file_table)
            {
                entry.read(tr);
                entry.file_path = path;

                // Prepend container stem to directory_name
                std::filesystem::path container(entry.container_name);
                auto stem = container.stem().string();
                entry.directory_name = stem + "/" + entry.directory_name;
            }

            return root;
        }
    };

    // -------------------------------------------------------
    //  FileSizeStats
    // -------------------------------------------------------
    struct FileSizeStats
    {
        uint32_t count_duplicated{0};
        uint32_t count_unique{0};
        uint32_t compressed_lowest{0};
        uint32_t compressed_highest{0};
        uint32_t compressed_avg{0};
        uint32_t uncompressed_lowest{0};
        uint32_t uncompressed_highest{0};
        uint32_t uncompressed_avg{0};
    };

    inline FileSizeStats compute_ipf_file_stats(const std::vector<IPFRoot> &ipfs)
    {
        uint64_t compressed_sum = 0;
        uint64_t uncompressed_sum = 0;
        uint32_t count = 0;
        uint32_t compressed_lo = UINT32_MAX;
        uint32_t compressed_hi = 0;
        uint32_t uncompressed_lo = UINT32_MAX;
        uint32_t uncompressed_hi = 0;

        for (auto &ipf : ipfs)
            for (auto &f : ipf.file_table)
            {
                ++count;
                compressed_sum += f.file_size_compressed;
                uncompressed_sum += f.file_size_uncompressed;
                compressed_lo = std::min(compressed_lo, f.file_size_compressed);
                compressed_hi = std::max(compressed_hi, f.file_size_compressed);
                uncompressed_lo = std::min(uncompressed_lo, f.file_size_uncompressed);
                uncompressed_hi = std::max(uncompressed_hi, f.file_size_uncompressed);
            }

        if (count == 0)
            return {};

        return FileSizeStats{
            count,
            0,
            compressed_lo,
            compressed_hi,
            static_cast<uint32_t>(compressed_sum / count),
            uncompressed_lo,
            uncompressed_hi,
            static_cast<uint32_t>(uncompressed_sum / count),
        };
    }

    // -------------------------------------------------------
    //  Parallel batch parsing
    // -------------------------------------------------------

    // Parse all .ipf files in a directory with up to max_threads workers.
    inline std::vector<IPFRoot> parse_all_ipf_files_limited_threads(
        const std::filesystem::path &dir, unsigned max_threads = 4)
    {
        std::vector<std::filesystem::path> paths;
        for (auto &entry : std::filesystem::directory_iterator(dir))
            if (entry.path().extension() == ".ipf")
                paths.push_back(entry.path());

        std::mutex paths_mutex, results_mutex;
        std::size_t next_job = 0;
        std::vector<IPFRoot> results;

        auto worker = [&]()
        {
            for (;;)
            {
                std::filesystem::path p;
                {
                    std::scoped_lock lock(paths_mutex);
                    if (next_job >= paths.size())
                        return;
                    p = paths[next_job++];
                }
                try
                {
                    auto root = IPFRoot::from_file(p);
                    std::scoped_lock lock(results_mutex);
                    results.push_back(std::move(root));
                }
                catch (const std::exception &e)
                {
                    std::cerr << "IPF: failed " << p << ": " << e.what() << '\n';
                }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(max_threads);
        for (unsigned i = 0; i < max_threads; ++i)
            threads.emplace_back(worker);
        for (auto &t : threads)
            t.join();

        return results;
    }

    // Parse game data/ and patch/ folders, merge results.
    inline std::vector<IPFRoot> parse_game_ipfs(
        const std::filesystem::path &game_root, unsigned max_threads = 4)
    {
        auto data_dir = game_root / "data";
        auto patch_dir = game_root / "patch";

        auto fut_data = std::async(std::launch::async, [&]
                                   { return parse_all_ipf_files_limited_threads(data_dir, max_threads); });
        auto fut_patch = std::async(std::launch::async, [&]
                                    { return parse_all_ipf_files_limited_threads(patch_dir, max_threads); });

        auto all = fut_data.get();
        auto patch = fut_patch.get();
        all.insert(all.end(),
                   std::make_move_iterator(patch.begin()),
                   std::make_move_iterator(patch.end()));
        return all;
    }

    // Drain all file tables out of parsed roots into a flat vector.
    inline std::vector<IPFFileTable> collect_file_tables(std::vector<IPFRoot> &parsed_ipfs)
    {
        std::vector<IPFFileTable> all;
        for (auto &root : parsed_ipfs)
        {
            all.insert(all.end(),
                       std::make_move_iterator(root.file_table.begin()),
                       std::make_move_iterator(root.file_table.end()));
            root.file_table.clear();
        }
        return all;
    }

    // Sort: by parent folder first, then by filename.
    inline void sort_file_tables_by_folder_then_name(std::vector<IPFFileTable> &tables)
    {
        std::sort(tables.begin(), tables.end(),
                  [](const IPFFileTable &a, const IPFFileTable &b)
                  {
                      if (!a.file_path || !b.file_path)
                          return false;
                      auto pa = a.file_path->parent_path();
                      auto pb = b.file_path->parent_path();
                      if (pa != pb)
                          return pa < pb;
                      return a.file_path->filename() < b.file_path->filename();
                  });
    }

    // Group file tables by directory_name.
    inline std::map<std::string, std::vector<IPFFileTable>>
    group_file_tables_by_directory(std::vector<IPFFileTable> tables)
    {
        std::map<std::string, std::vector<IPFFileTable>> result;
        for (auto &f : tables)
            result[f.directory_name].push_back(std::move(f));
        return result;
    }

    // -------------------------------------------------------
    //  Hex dump utility
    // -------------------------------------------------------
    inline void print_hex_viewer(const uint8_t *data, std::size_t size, std::ostream &os = std::cout)
    {
        constexpr std::size_t BPL = 16;
        for (std::size_t i = 0; i < size; i += BPL)
        {
            os << std::dec;
            os.fill('0');
            os.width(8);
            os << i << "  ";
            for (std::size_t j = 0; j < BPL; ++j)
            {
                if (i + j < size)
                {
                    char buf[4];
                    std::snprintf(buf, sizeof(buf), "%02X ", data[i + j]);
                    os << buf;
                }
                else
                    os << "   ";
            }
            os << ' ';
            for (std::size_t j = 0; j < BPL && i + j < size; ++j)
            {
                uint8_t b = data[i + j];
                os << (std::isprint(b) ? static_cast<char>(b) : '.');
            }
            os << '\n';
        }
    }

    inline void print_hex_viewer(const std::vector<uint8_t> &data, std::ostream &os = std::cout)
    {
        print_hex_viewer(data.data(), data.size(), os);
    }

} // namespace ipf

#endif // IPF_HPP