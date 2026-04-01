#pragma once
#ifndef IES_PRINT_HPP
#define IES_PRINT_HPP

#include "ies.hpp"
#include <iostream>
#include <iomanip>

namespace ies
{

    // Helper to print a vector
    template <typename T>
    void print_vector(std::ostream &os, const std::vector<T> &vec, const std::string &name)
    {
        os << name << " [size=" << vec.size() << "]\n";
        for (size_t i = 0; i < vec.size(); ++i)
        {
            os << "  [" << i << "] " << vec[i] << "\n";
        }
    }

    // ------------------------
    // operator<< for structs
    // ------------------------
    inline std::ostream &operator<<(std::ostream &os, const IESColumn &col)
    {
        os << "{ column: \"" << col.column << "\", name: \"" << col.name
           << "\", type_data: " << col.type_data
           << ", access_data: " << col.access_data
           << ", sync_data: " << col.sync_data
           << ", decl_idx: " << col.decl_idx << " }";
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const IESRowText &t)
    {
        os << "{ text_data: \"" << t.text_data << "\" }";
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const IESRowFloat &f)
    {
        os << "{ float_data: " << f.float_data << " }";
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const IESColumnData &d)
    {
        os << "{ index_data: " << d.index_data << ", row_text: " << d.row_text << "\n";
        print_vector(os, d.floats, "floats");
        print_vector(os, d.texts, "texts");
        print_vector(os, d.padding, "padding");
        os << "}";
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const IESHeader &h)
    {
        os << "{ idspace: \"" << h.idspace << "\", keyspace: \"" << h.keyspace
           << "\", version: " << h.version
           << ", info_size: " << h.info_size
           << ", data_size: " << h.data_size
           << ", total_size: " << h.total_size
           << ", use_class_id: " << int(h.use_class_id)
           << ", num_field: " << h.num_field
           << ", num_column: " << h.num_column
           << ", num_column_number: " << h.num_column_number
           << ", num_column_string: " << h.num_column_string
           << " }";
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const IESRoot &root)
    {
        os << "IESRoot:\n";
        os << "Header: " << root.header << "\n";

        os << "Columns:\n";
        for (size_t i = 0; i < root.columns.size(); ++i)
        {
            os << "  [" << i << "] " << root.columns[i] << "\n";
        }

        os << "Data:\n";
        for (size_t i = 0; i < root.data.size(); ++i)
        {
            os << "  [" << i << "] " << root.data[i] << "\n";
        }

        return os;
    }

} // namespace ies

#endif // IES_PRINT_HPP