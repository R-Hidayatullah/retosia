#include "ies.hpp"
#include "ies_print.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
struct Utf8Console
{
    Utf8Console() { SetConsoleOutputCP(CP_UTF8); }
} utf8console;
#endif

int main()
{
    try
    {
        auto root = ies::IESRoot::from_file("tests/cell.ies");
        std::cout << root << "\n";
    }
    catch (const binreader::BinaryReadError &e)
    {
        std::cerr << "Binary read failed: " << e.what() << "\n";
    }
}