#pragma once
#ifndef BINARY_READER_HPP
#define BINARY_READER_HPP

#include <cstdint>
#include <vector>
#include <fstream>
#include <string>
#include <stdexcept>
#include <concepts>
#include <type_traits>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace binreader
{

    // =========================
    // Custom exception
    // =========================
    class BinaryReadError : public std::runtime_error
    {
    public:
        BinaryReadError(const std::string &msg, std::size_t offset = 0, std::size_t requested = 0)
            : std::runtime_error(format(msg, offset, requested)), offset_(offset), requested_(requested) {}

        std::size_t offset() const noexcept { return offset_; }
        std::size_t requested() const noexcept { return requested_; }

    private:
        std::size_t offset_;
        std::size_t requested_;

        static std::string format(const std::string &msg, std::size_t offset, std::size_t requested)
        {
            std::ostringstream ss;
            ss << msg << " | offset: 0x" << std::hex << offset
               << " | requested: " << std::dec << requested << " bytes";
            return ss.str();
        }
    };

    // =========================
    // BinaryReader class
    // =========================
    class BinaryReader
    {
    public:
        // ---- Constructors ----
        BinaryReader(std::vector<uint8_t> data)
            : buffer_(std::move(data)), pos_(0), owns_buffer_(true) {}

        BinaryReader(const std::string &filename, std::size_t buffer_size = 4096)
            : file_(filename, std::ios::binary), buffer_(buffer_size), pos_(0), owns_buffer_(false)
        {
            if (!file_)
                throw BinaryReadError("Cannot open file: " + filename);
            fill_buffer();
        }

        // ---- Disable copy ----
        BinaryReader(const BinaryReader &) = delete;
        BinaryReader &operator=(const BinaryReader &) = delete;

        // ---- Moveable ----
        BinaryReader(BinaryReader &&) noexcept = default;
        BinaryReader &operator=(BinaryReader &&) noexcept = default;

        // ---- Read arithmetic types (integral or floating) ----
        template <typename T>
        T read()
        {
            static_assert(std::is_arithmetic_v<T>, "BinaryReader::read<T> requires arithmetic type");
            T value{};
            read_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
            return value;
        }

        // ---- Read arbitrary bytes ----
        void read_bytes(uint8_t *dest, std::size_t size)
        {
            if (owns_buffer_)
            {
                if (pos_ + size > buffer_.size())
                    throw BinaryReadError("Out of bounds read in memory buffer", pos_, size);
                std::copy(buffer_.begin() + pos_, buffer_.begin() + pos_ + size, dest);
                pos_ += size;
            }
            else
            {
                std::size_t read_count = 0;
                while (read_count < size)
                {
                    if (pos_ >= buffer_filled_)
                    {
                        fill_buffer();
                        if (buffer_filled_ == 0)
                            throw BinaryReadError("Unexpected end of file", pos_, size - read_count);
                    }
                    std::size_t to_copy = std::min(size - read_count, buffer_filled_ - pos_);
                    std::copy(buffer_.begin() + pos_, buffer_.begin() + pos_ + to_copy, dest + read_count);
                    pos_ += to_copy;
                    read_count += to_copy;
                }
            }
        }

        // ---- Seek ----
        void seek(std::size_t offset)
        {
            if (owns_buffer_)
            {
                if (offset > buffer_.size())
                    throw BinaryReadError("Seek out of bounds in memory buffer", offset);
                pos_ = offset;
            }
            else
            {
                file_.seekg(offset, std::ios::beg);
                if (!file_)
                    throw BinaryReadError("Failed to seek file", offset);
                pos_ = buffer_filled_ = 0;
            }
        }

        std::size_t tell() const { return pos_; }

        bool eof() const
        {
            if (owns_buffer_)
                return pos_ >= buffer_.size();
            return file_.eof() && pos_ >= buffer_filled_;
        }

    private:
        void fill_buffer()
        {
            if (!file_)
                return;
            file_.read(reinterpret_cast<char *>(buffer_.data()), buffer_.size());
            buffer_filled_ = static_cast<std::size_t>(file_.gcount());
            pos_ = 0;
        }

        // ---- Members ----
        std::ifstream file_;
        std::vector<uint8_t> buffer_;
        std::size_t pos_ = 0;
        std::size_t buffer_filled_ = 0;
        bool owns_buffer_ = false;
    };

} // namespace binreader

#endif // BINARY_READER_HPP