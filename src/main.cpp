#include "binary_reader.hpp"
#include <iostream>

int main()
{
    // From memory
    std::vector<uint8_t> data = {0x01, 0x00, 0x00, 0x00};
    binreader::BinaryReader reader(data);
    uint32_t val = reader.read<uint32_t>();
    std::cout << "Value: " << val << "\n"; // 1

    // From file
    binreader::BinaryReader fileReader("C:\\Users\\Ridwan Hidayatullah\\Documents\\tosmole\\tests\\skilvoice_jap.fsb");
    uint8_t buffer[4];
    fileReader.read_bytes(buffer, 4);

    std::cout << "Bytes: ";
    for (auto b : buffer)
        std::cout << char(b) << " ";
    std::cout << "\n";
}