#include <iostream>
#include <fstream>
#include "tok.hpp" // your TokParser and TokNode definitions

int main()
{
    try
    {
        // -----------------------------
        // 1. Parse TOK file
        // -----------------------------
        tok::TokParser parser("tests/barrack4.tok"); // replace with your file path
        tok::TokNode root = parser.parse();

        // -----------------------------
        // 2. Print TOK tree for debugging
        // -----------------------------
        tok::print_tok_tree(std::cout, root);

        // -----------------------------
        // 3. Export mesh to SVG
        // -----------------------------
        std::ofstream svg_file("output.svg");
        if (!svg_file)
            throw std::runtime_error("Cannot create SVG file");

        tok::export_to_svg(root, svg_file, 600.0f, 600.0f);
        std::cout << "SVG exported successfully to output.svg\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}