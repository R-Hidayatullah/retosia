#if !defined(BINARY_READER_HPP)
#define BINARY_READER_HPP

#include <cstdint>
#include <vector>
#include <fstream>
#include <string>
#include <stdexcept>
#include <concepts>
#include <type_traits>

namespace binreader
{

    class BinaryReader
    {
    public:
        // Constructors
        BinaryReader(std::vector<uint8_t> data)
            : buffer_(std::move(data)), pos_(0), owns_buffer_(true) {}

        BinaryReader(const std::string &filename, std::size_t buffer_size = 4096)
            : file_(filename, std::ios::binary), buffer_(buffer_size), pos_(0), owns_buffer_(false)
        {
            if (!file_)
                throw std::runtime_error("Cannot open file: " + filename);
            fill_buffer();
        }

        // Disable copy
        BinaryReader(const BinaryReader &) = delete;
        BinaryReader &operator=(const BinaryReader &) = delete;

        // Moveable
        BinaryReader(BinaryReader &&) noexcept = default;
        BinaryReader &operator=(BinaryReader &&) noexcept = default;

        // Read arithmetic types
        template <std::integral T>
        T read()
        {
            T value{};
            read_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
            return value;
        }

        template <std::floating_point T>
        T read()
        {
            T value{};
            read_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
            return value;
        }

        // Read arbitrary bytes
        void read_bytes(uint8_t *dest, std::size_t size)
        {
            if (owns_buffer_)
            {
                if (pos_ + size > buffer_.size())
                    throw std::runtime_error("Out of bounds read in memory buffer");
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
                            throw std::runtime_error("Unexpected end of file");
                    }
                    std::size_t to_copy = std::min(size - read_count, buffer_filled_ - pos_);
                    std::copy(buffer_.begin() + pos_, buffer_.begin() + pos_ + to_copy, dest + read_count);
                    pos_ += to_copy;
                    read_count += to_copy;
                }
            }
        }

        // Seek in memory or file
        void seek(std::size_t offset)
        {
            if (owns_buffer_)
            {
                if (offset > buffer_.size())
                    throw std::runtime_error("Seek out of bounds in memory buffer");
                pos_ = offset;
            }
            else
            {
                file_.seekg(offset, std::ios::beg);
                pos_ = buffer_filled_ = 0;
            }
        }

        std::size_t tell() const
        {
            return pos_;
        }

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

        // Members
        std::ifstream file_;
        std::vector<uint8_t> buffer_;
        std::size_t pos_ = 0;
        std::size_t buffer_filled_ = 0;
        bool owns_buffer_ = false;
    };

} // namespace binreader

#endif // BINARY_READER_HPP
